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


#include <evpaxos.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

struct client_value
{
	int client_id;
	struct timeval t;
	size_t size;
	char value[0];
};

static int verbose = 0;

/// <summary>
/// Output on incoming signals and exit loop.
/// </summary>
/// <param name="sig">=> Incoming signal</param>
/// <param name="ev">=> No usage found</param>
/// <param name="arg">=> More Arguments; Base</param>
static void handle_sigint(int sig, short ev, void* arg)
{
	struct event_base* base = arg;
	printf("Caught signal %d\n", sig);
	event_base_loopexit(base, NULL);
}

/// <summary>
/// 
/// </summary>
/// <param name="iid">=> Instance ID</param>
/// <param name="value">=> Value to propose</param>
/// <param name="size">=> Size of value that is moved</param>
/// <param name="arg">=> more Arguments; No usage found.</param>
static void deliver(unsigned iid, char* value, size_t size, void* arg)
{
	struct client_value* val = (struct client_value*)value;
	printf("%ld.%06ld[%.16s] %ld bytes\n", val->t.tv_sec, val->t.tv_usec,
		val->value, (long)val->size);
}

/// <summary>
/// Set up and start replica. It reads the configuration with evpaxos_config_read (config.c) and initialize it with ecpaxos_replica_init ().
/// </summary>
/// <param name="id">Replica ID</param>
/// <param name="config">Path of paxos.conf</param>
static void start_replica(int id, const char* config)
{
	struct event* sig;
	struct event_base* base;
	struct evpaxos_replica* replica;
	deliver_function cb = NULL;
	struct evpaxos_config* cfg;

	if (verbose)
		cb = deliver;
	base = event_base_new();
	cfg = evpaxos_config_read(config);

	replica = evpaxos_replica_init(id, cfg, cb, NULL, base);

	if (replica == NULL) {
		printf("Could not start the replica!\n");
		exit(1);
	}

	sig = evsignal_new(base, SIGINT, handle_sigint, base);
	evsignal_add(sig, NULL);

	signal(SIGPIPE, SIG_IGN);
	event_base_dispatch(base);

	event_free(sig);
	evpaxos_replica_free(replica);
	event_base_free(base);
	evpaxos_config_free(cfg);
}

/// <summary>
/// It shows the correct usage/call.
/// </summary>
/// <param name="prog">Program that is called from.</param>
static void usage(const char* prog)
{
	printf("Usage: %s id [path/to/paxos.conf] [-h] [-s]\n", prog);
	printf("  %-30s%s\n", "-h, --help", "Output this message and exit");
	printf("  %-30s%s\n", "-v, --verbose", "Print delivered messages");
	exit(1);
}

/// <summary>
/// It is a main function solely to run a stand-alone replica.
/// 
/// Example: ./sample/replica 0 ../paxos.conf
/// </summary>
/// <param name="argc">Number of parameter for the main function.</param>
/// <param name="argv">Array of all paramter given to the main function</param>
/// <returns>An int value that shows if main is successful.</returns>
int main(int argc, char const* argv[])
{
	int id;
	int i = 2;
	const char* config = "../paxos.conf";

	/*
	* argv[0] => Program itself.
	* argv[1] => ID for replica. It works as position in paxos.conf at the same time
	* argv[2] => Path to paxos.conf;
	*/
	if (argc < 2)
		usage(argv[0]);
	id = atoi(argv[1]);
	if (argc >= 3 && argv[2][0] != '-') {
		config = argv[2];
		printf("argc length: %d\ncurrent i: %d\nconfig: %s\n", argc, i, config);
		i++;
	}
	printf("\nLast segment.");
	while (i != argc) { // It checks if -h or -v are given as additional parameters. If there too many parameters or -h (independent of number of parameters) then it show how it is used.
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
			usage(argv[0]);
		else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
			verbose = 1;
		else
			usage(argv[0]);
		i++;
	}
	printf("\nStart Replica.");
	start_replica(id, config); // Start process for replica.
	printf("finished\n");
	return 0;
}
