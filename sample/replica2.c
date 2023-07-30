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


struct client_value
{
	int client_id;
	struct timeval t;
	size_t size;
	char value[0];
};

static int verbose = 0;

static void
handle_sigint(int sig, short ev, void* arg)
{
	struct event_base* base = arg;
	printf("Caught signal %d\n", sig);
	event_base_loopexit(base, NULL);
}

static void
deliver(unsigned iid, char* value, size_t size, void* arg)
{
	struct client_value* val = (struct client_value*)value;
	printf("%ld.%06ld [%.16s] %ld bytes\n", val->t.tv_sec, val->t.tv_usec,
		val->value, (long)val->size);
}

static void
start_replica(int nnodes, struct evpaxos_config* cfg, pthread_t* ref, pthread_mutex_t* syncs, void** cs)
{
	struct event* sig;
	struct event_base* base;
	deliver_function cb = NULL;
	struct evpaxos_parms* p;
	if (verbose)
		cb = deliver;
	base = event_base_new();
	int i = 0;
	while (i < nnodes)
	{
		p = evpaxos_alloc_parms(i, cfg, cb, NULL, base, &(syncs[i]));
		cs[i] = (void*)p;

		evpaxos_replica_init_thread(&(ref[i]), &(syncs[i]), p);
		++i;
	}

	sig = evsignal_new(base, SIGINT, handle_sigint, base);
	evsignal_add(sig, NULL);
	signal(SIGPIPE, SIG_IGN);
	event_base_dispatch(base);
	event_free(sig);
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

static void
usage(const char* prog)
{
	printf("Usage: %s [path/to/paxos.conf] [-h] [-s]\n", prog);
	printf("  %-30s%s\n", "-h, --help", "Output this message and exit");
	printf("  %-30s%s\n", "-v, --verbose", "Print delivered messages");
	exit(1);
}

int
main(int argc, char const* argv[])
{
	int i = 1;
	const char* config = "../paxos.conf";

	if (argc < 1)
		usage(argv[0]);

	if (argc >= 2 && argv[1][0] != '-') {
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



	struct evpaxos_config* cfg;
	cfg = evpaxos_config_read(config);
	if (cfg < 0) return 0;

	int nnodes = evpaxos_replica_nodes(cfg);
	printf("path %s nodes %d", config, nnodes);
	fflush(stdout);


	pthread_t* threads = calloc(nnodes,sizeof(pthread_t));
	pthread_mutex_t* syncs = calloc(nnodes, sizeof(pthread_mutex_t));
	void** cs = calloc(nnodes, sizeof(void*));

	start_replica(nnodes, cfg, threads, syncs, cs);

	free(threads);
	free(syncs);

	return 0;
}
