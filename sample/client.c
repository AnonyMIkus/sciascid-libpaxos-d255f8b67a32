/*
 * Copyright (c) 2013-2015, University of Lugano
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holders nor the names of it
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <paxos.h>
#include <evpaxos.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <event2/event.h>
#include <netinet/tcp.h>
#include <malloc.h>
#include <pthread.h>

#define MAX_VALUE_SIZE 8192


struct client_value
{
	int client_id;
	struct timeval t;
	size_t size;
	char value[0];
};

struct stats
{
	long min_latency;
	long max_latency;
	long avg_latency;
	int delivered_count;
	size_t delivered_bytes;
};

struct client
{
	int id;
	int value_size;
	int outstanding; // Number of Request??
	char* send_buffer;
	struct stats stats;
	struct event_base* base;
	struct bufferevent* bev;
	struct event* stats_ev;
	struct timeval stats_interval;
	struct event* sig;
	struct evlearner* learner;
};

/**
 * This function handles the SIGINT signal (typically generated by pressing Ctrl+C)
 * and prints a message before exiting the event loop associated with the provided event base.
 *
 * @param sig The signal number (SIGINT).
 * @param ev The event information (unused).
 * @param arg The event base pointer.
 */
static void handle_sigint(int sig, short ev, void* arg)
{
	struct event_base* base = arg;
	printf("Caught signal %d\n", sig);
	event_base_loopexit(base, NULL);
}

/**
 * This function generates a random string of the given length using a set of alphanumeric characters.
 * The generated string is null-terminated and stored in the provided character array.
 *
 * @param s The character array to store the generated random string.
 * @param len The length of the random string to generate.
 */
static void random_string(char *s, const int len)
{
	int i;
	static const char alphanum[] = "0123456789abcdefghijklmnopqrstuvwxyz";

	for (i = 0; i < len-1; ++i)
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];

	s[len-1] = 0;
	// No return here because we get string-on-reference.
}

/**
 * This function prepares and submits a client value to the distributed system.
 * It includes generating a random value, associating it with a client ID and timestamp,
 * and submitting it using the provided event buffer.
 *
 * @param c A pointer to the client structure.
 */
static void client_submit_value(struct client* c)
{
	//paxos_log_debug("submitting paxos value");
	struct client_value* v = (struct client_value*)c->send_buffer;
	v->client_id = c->id;
	gettimeofday(&v->t, NULL);
	v->size = c->value_size;
	random_string(v->value, v->size);
	size_t size = sizeof(struct client_value) + v->size;
	paxos_submit(c->bev, c->send_buffer, size);
}

// Returns t2 - t1 in microseconds.

/**
 * @brief Calculate the time difference between two timeval structures.
 *
 * This function calculates the time difference in microseconds between two timeval structures.
 * It is used to measure the latency between two timestamps.
 *
 * @param t1 A pointer to the first timeval structure.
 * @param t2 A pointer to the second timeval structure.
 * @return The time difference in microseconds (t2 - t1).
 */
static long timeval_diff(struct timeval* t1, struct timeval* t2)
{
	long us;
	us = (t2->tv_sec - t1->tv_sec) * 1e6;

	if (us < 0)
		return 0;

	us += (t2->tv_usec - t1->tv_usec);
	return us;
}

/**
 * This function updates client statistics with information from delivered client values.
 * It tracks the number of delivered values, delivered bytes, and calculates statistics such as
 * average latency, minimum latency, and maximum latency.
 *
 * @param stats A pointer to the client statistics structure.
 * @param delivered A pointer to the delivered client value.
 * @param size The size of the delivered client value.
 */
static void update_stats(struct stats* stats, struct client_value* delivered, size_t size)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long lat = timeval_diff(&delivered->t, &tv);

	// Calculate statistical values.
	stats->delivered_count++;
	stats->delivered_bytes += size;
	stats->avg_latency = stats->avg_latency + ((lat - stats->avg_latency) / stats->delivered_count);

	if (stats->min_latency == 0 || lat < stats->min_latency)
		stats->min_latency = lat;

	if (lat > stats->max_latency)
		stats->max_latency = lat;
}

/**
 * This function processes delivered consensus messages, updates client statistics, and resubmits client values
 * if the delivered message matches the client's ID.
 *
 * @param iid The instance ID of the delivered consensus message.
 * @param value The consensus message value.
 * @param size The size of the consensus message.
 * @param arg A pointer to the client structure.
 */
static void on_deliver(unsigned iid, char* value, size_t size, void* arg)
{
	struct client* c = arg;
	struct client_value* v = (struct client_value*)value;

	if (v->client_id == c->id) {
		update_stats(&c->stats, v, size);
		client_submit_value(c);
	}
}

/**
 * This function prints client statistics, including the number of delivered values, delivered bytes,
 * average latency, minimum latency, and maximum latency. It then resets the statistics and schedules
 * another callback to report statistics periodically.
 *
 * @param fd The file descriptor (unused).
 * @param event The event information (unused).
 * @param arg A pointer to the client structure.
 */

unsigned long etprev = 0;

static void on_stats(evutil_socket_t fd, short event, void* arg)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	unsigned long nsec = tv.tv_sec - etprev;
	unsigned long tsec = tv.tv_sec;
	struct client* c = arg;

	if (nsec != 0)
	{
		etprev = tsec;

		double mbps = (double)(c->stats.delivered_bytes * 8) / (1024 * 1024);
	// printf("%d value/sec, %.2f Mbps, latency min %ld us max %ld us avg %ld us\n", c->stats.delivered_count, mbps, c->stats.min_latency,	c->stats.max_latency, c->stats.avg_latency);
		printf("%d;%ld;%ld;%ld\n", c->stats.delivered_count, c->stats.min_latency, c->stats.max_latency, c->stats.avg_latency);
		FILE* pf; pf = fopen("statsclient5.csv", "a+");
		char Buff[1024]; memset(Buff, 0, sizeof(Buff));

		int off = strftime(Buff, sizeof(Buff), "%d %b %H:%M:%S;", localtime(&tv.tv_sec));

		sprintf(Buff + off, "$ld,%d;%ld;%ld;%ld\n",nsec,c->stats.delivered_count, c->stats.min_latency, c->stats.max_latency, c->stats.avg_latency);
		fputs(Buff, pf);
		fflush(pf);
		fclose(pf);
    }
	memset(&c->stats, 0, sizeof(struct stats));
	event_add(c->stats_ev, &c->stats_interval);
}


/**
 * This function is called when a connection event occurs (e.g., connected to a proposer or error).
 * It prints a message upon successful connection and handles connection errors.
 *
 * @param bev The bufferevent associated with the connection.
 * @param events The event flags indicating the type of event.
 * @param arg A pointer to the client structure.
 */
static void on_connect(struct bufferevent* bev, short events, void* arg)
{
	int i;
	struct client* c = arg;

	if (events & BEV_EVENT_CONNECTED) {
		paxos_log_debug("Connected to proposer");

		for (i = 0; i < c->outstanding; ++i)
			client_submit_value(c);
	} else {
		printf("%s\n", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
	}
}

/**
 * This function establishes a connection to a proposer using configuration information
 * and returns the bufferevent associated with the connection.
 *
 * @param c A pointer to the client structure.
 * @param config The path to the configuration file for EvPaxos.
 * @param proposer_id The ID of the proposer to connect to.
 * @return A pointer to the bufferevent for the connection, or NULL on failure.
 */
static struct bufferevent* connect_to_proposer(struct client* c, const char* config, int proposer_id)
{
	struct bufferevent* bev; // Socket and attributes to it.
	struct evpaxos_config* conf = evpaxos_config_read(config);

	if (conf == NULL) {
		printf("Failed to read config file %s\n", config);
		return NULL;
	}

	struct sockaddr_in addr = evpaxos_proposer_address(conf, proposer_id);
	// Set up bufferevent that is threadsafe.
	bev = bufferevent_socket_new(c->base, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_UNLOCK_CALLBACKS | BEV_OPT_THREADSAFE);
	bufferevent_setcb(bev, NULL, NULL, on_connect, c);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
	bufferevent_socket_connect(bev, (struct sockaddr*)&addr, sizeof(addr));
	int flag = 1;
	setsockopt(bufferevent_getfd(bev), IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
	paxos_log_debug("Connected to Proposer %u", proposer_id);
	return bev;
}

/**
 * This function creates and initializes a client structure with the specified parameters,
 * including the number of outstanding values, value size, and proposer ID. It also sets up
 * the necessary event handling for statistics reporting and signal handling.
 *
 * @param config The path to the configuration file for EvPaxos.
 * @param proposer_id The ID of the proposer to connect to.
 * @param outstanding The number of outstanding client values.
 * @param value_size The size of client values (in bytes).
 * @return A pointer to the initialized client structure.
 */
static struct client* make_client(const char* config, int proposer_id, int outstanding, int value_size)
{
	struct client* c;
	c = malloc(sizeof(struct client));
	// Threadsafe event call.
	struct event_config* event_config = event_config_new();
	c->base = event_base_new_with_config(event_config); // Have to look up event_base_new() / libevent.
	event_config_free(event_config);
	memset(&c->stats, 0, sizeof(struct stats));
	c->bev = connect_to_proposer(c, config, proposer_id);

	if (c->bev == NULL) // Exit if proposer do not exist.
		exit(1);
	
	// Set up client if client could connect.
	c->id = rand(); // Get random ID.
	c->value_size = value_size;
	c->outstanding = outstanding;
	c->send_buffer = malloc(sizeof(struct client_value) + value_size);
	c->stats_interval = (struct timeval){1, 0};
	c->stats_ev = evtimer_new(c->base, on_stats, c);
	event_add(c->stats_ev, &c->stats_interval);
	paxos_config.learner_catch_up = 0;
	c->learner = evlearner_init(config, on_deliver, c, c->base); // Assigning a learner to the client.
	
	// Signal handling
	c->sig = evsignal_new(c->base, SIGINT, handle_sigint, c->base);
	evsignal_add(c->sig, NULL);
	return c;
}

/**
 * This function releases the resources associated with a client, including memory allocations
 * and event structures.
 *
 * @param c A pointer to the client structure to be freed.
 */
static void client_free(struct client* c)
{
	free(c->send_buffer);
	bufferevent_free(c->bev);
	event_free(c->stats_ev);
	event_free(c->sig);
	event_base_free(c->base);

	if (c->learner)
		evlearner_free(c->learner);

	free(c);
}

/**
 * This function starts the client application with the specified configuration parameters.
 * It sets up signal handling and initiates the event loop for the client.
 *
 * @param config The path to the configuration file for EvPaxos.
 * @param proposer_id The ID of the proposer to connect to.
 * @param outstanding The number of outstanding client values.
 * @param value_size The size of client values (in bytes).
 */
static void start_client(const char* config, int proposer_id, int outstanding, int value_size)
{
	struct client* client;
	client = make_client(config, proposer_id, outstanding, value_size);
	signal(SIGPIPE, SIG_IGN);
	event_base_dispatch(client->base);
	client_free(client);
}

/**
 * This function prints usage information and command-line options for the client application.
 *
 * @param name The name of the client application executable.
 */
static void usage(const char* name)
{
	printf("Usage: %s [path/to/paxos.conf] [-h] [-o] [-v] [-p]\n", name);
	printf("  %-30s%s\n", "-h, --help", "Output this message and exit");
	printf("  %-30s%s\n", "-o, --outstanding #", "Number of outstanding client values");
	printf("  %-30s%s\n", "-v, --value-size #", "Size of client value (in bytes)");
	printf("  %-30s%s\n", "-p, --proposer-id #", "d of the proposer to connect to");
	exit(1);
}

int main(int argc, char const *argv[])
{
	mallopt(M_MXFAST, 0);
//	mallopt(M_PERTURB, 0x100);
	evthread_use_pthreads();
	int i = 1;
	int proposer_id = 0;
	int outstanding = 1;
	int value_size = 64;
	struct timeval seed;
	const char* config = "../paxos.conf";

	if (argc > 1 && argv[1][0] != '-') {
		config = argv[1];
		i++;
	}

	// sleep(30);

	//Iterating through all parameters in prompt.
	while (i != argc) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
			usage(argv[0]);
		else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--outstanding") == 0)
			outstanding = atoi(argv[++i]);
		else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--value-size") == 0)
			value_size = atoi(argv[++i]);
		else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--proposer-id") == 0)
			proposer_id = atoi(argv[++i]);
		else
			usage(argv[0]);
		i++;
	}

	gettimeofday(&seed, NULL);
	srand(seed.tv_usec);
	start_client(config, proposer_id, outstanding, value_size);
	return 0;
}
