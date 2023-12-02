/*
 * Copyright (c) 2014-2015, University of Lugano
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

#define MAX_N_OF_THREADS 100

#include <evpaxos.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <malloc.h>


struct client_value
{
	int client_id;
	struct timeval t;
	size_t size;
	char value[0];
};

static int verbose = 0;

/**
 * This function handles the SIGINT signal (typically generated by pressing
 * Ctrl+C) and initiates the termination of the replica by calling
 * event_base_loopexit.
 *
 * @param sig The signal number (SIGINT).
 * @param ev The event information (unused).
 * @param arg The event base pointer.
 */
static void handle_sigint(int sig, short ev, void* arg)
{
	struct event_base* base = arg;
	paxos_log_debug("Caught signal %d", sig);
	event_base_loopexit(base, NULL);
}

/**
 * This function is called when a consensus message is delivered to the replica.
 * It logs the received message and its timestamp.
 *
 * @param iid The instance ID of the consensus message.
 * @param value The consensus message value.
 * @param size The size of the consensus message.
 * @param arg User-defined argument (unused).
 */
static void deliver(unsigned iid, char* value, size_t size, void* arg)
{
	struct client_value* val = (struct client_value*)value;
	paxos_log_debug("%ld.%06ld [%.16s] %ld bytes", val->t.tv_sec, val->t.tv_usec, val->value, (long)val->size);
}

/**
 * This function initializes and starts the EvPaxos-based replica. It creates
 * multiple threads, each representing a replica, and sets up signal handling
 * for SIGINT (Ctrl+C).
 *
 * @param nnodes The number of replica nodes.
 * @param cfg The EvPaxos configuration.
 * @param ref An array of thread IDs for replica threads.
 * @param syncs An array of thread synchronization mutexes.
 * @param cs An array of EvPaxos parameters for each replica.
 * @param pgs A mutex for protecting the Paxos group state.
 */
static void start_replica(int nnodes, struct evpaxos_config* cfg, pthread_t* ref, pthread_mutex_t* syncs, void** cs, pthread_mutex_t* pgs)
{
	struct event* sig;
	struct event_base* base;
	deliver_function cb = NULL;
	struct evpaxos_parms* p;

	if (verbose)
		cb = deliver;

	struct event_config* event_config = event_config_new();
	base = event_base_new_with_config(event_config);
	setPGS(cfg, pgs);
	int i = 0;

	while (i < nnodes)
	{
		p = evpaxos_alloc_parms(i, cfg, cb, NULL, base, &(syncs[i]),pgs);
		cs[i] = (void*)p;
		paxos_log_debug("Init thread in parent");
		evpaxos_replica_init_thread(&(ref[i]), &(syncs[i]), p);
		paxos_log_debug("Init thread in parent finished");
		++i;
	}

	sig = evsignal_new(base, SIGINT, handle_sigint, base);
	evsignal_add(sig, NULL);
	signal(SIGPIPE, SIG_IGN);
	event_base_dispatch(base);
	event_free(sig);
	event_config_free(event_config);
	event_base_free(base);

	i = 0;
	while (i < nnodes) 
	{
		pthread_mutex_unlock(&(syncs[i]));
		pthread_join(ref[i], NULL);
		pthread_detach(ref[i]);
		free(cs[i]);
		evpaxos_config_free(cfg);
		++i;
	}
}

/**
 * This function is responsible for printing a usage message to the standard
 * output when the program's command-line arguments are incorrect or when
 * the user requests help using the `-h` or `--help` option.
 *
 * @param prog The name of the program (typically `argv[0]`).
 */
static void usage(const char* prog)
{
	printf("Usage: %s [path/to/paxos.conf] [-h] [-s]\n", prog);
	printf("  %-30s%s\n", "-h, --help", "Output this message and exit");
	printf("  %-30s%s\n", "-v, --verbose", "Print delivered messages");
	exit(1);
}

int main(int argc, char const* argv[])
{
	mallopt(M_MXFAST, 0);
	//	mallopt(M_PERTURB, 0x100);
	evthread_use_pthreads();
	int i = 1;
	const char* config = "../paxos.conf";

	// Analysis of input.
	if (argc < 1)
		usage(argv[0]);

	if (argc >= 2 && argv[1][0] != '-') 
	{
		config = argv[1];
		i++;
	}

	while (i != argc) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
			usage(argv[0]);
		else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
			verbose = 1;
		else
			usage(argv[0]);
		i++;
	}

	{
	char Buff[1024]; memset(Buff, 0, sizeof(Buff));
	sprintf(Buff, "time;pid;msg;timediff;replicas;msgpersec\n");
	FILE* pf;
	pf = fopen("msgstat.csv", "a+");
	fputs(Buff, pf);
	fflush(pf);
	fclose(pf);
	}
	// Process should wait for following reason: Allowing to start debug.
	// sleep(30);

	struct evpaxos_config* cfg;
	cfg = evpaxos_config_read(config);

	if (cfg < 0)
		return 0;

	int nnodes = evpaxos_replica_nodes(cfg);
	// paxos_log_debug("path %s nodes %d", config, nnodes);
	fflush(stdout);

	// Thread and Mutex
	pthread_t* threads = calloc(nnodes,sizeof(pthread_t));
	pthread_mutex_t* syncs = calloc(nnodes, sizeof(pthread_mutex_t));
	void** cs = calloc(nnodes, sizeof(void*));
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_t gs = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_init(&gs, &attr);
	pthread_mutexattr_destroy(&attr);

	start_replica(nnodes, cfg, threads, syncs, cs, &gs);
	free(threads);
	free(syncs);
	pthread_mutex_destroy(&gs);
	return 0;
}
