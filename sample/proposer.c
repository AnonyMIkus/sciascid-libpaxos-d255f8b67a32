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


#include <stdlib.h>
#include <stdio.h>
#include <evpaxos.h>
#include <signal.h>

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
/// Create a propser that ist waiting for events to handle it,
/// </summary>
/// <param name="config">Path to paxos.config</param>
/// <param name="id">Given ID</param>
static void start_proposer(const char* config, int id)
{
	struct event* sig;
	struct event_base* base;
	struct evproposer* prop;

	base = event_base_new();
	sig = evsignal_new(base, SIGINT, handle_sigint, base);
	evsignal_add(sig, NULL);
	
	prop = evproposer_init(id, config, base);
	if (prop == NULL) {
		printf("Could not start the proposer!\n");
		exit(1);
	}

	signal(SIGPIPE, SIG_IGN);
	event_base_dispatch(base);

	event_free(sig);
	evproposer_free(prop);
	event_base_free(base);
}

/// <summary>
/// It is a main function solely to run a stand-alone proposer.
/// 
/// Example: ./sample/proposer 0 ../paxos.conf
/// </summary>
/// <param name="argc">Number of parameter for the main function.</param>
/// <param name="argv">Array of all paramter given to the main function</param>
/// <returns>An int value that shows if main is successful.</returns>
int main (int argc, char const *argv[])
{
	int id;
	const char* config = "../paxos.conf";

	if (argc != 2 && argc != 3) {
		printf("Usage: %s id [path/to/paxos.conf]\n", argv[0]);
		exit(1);
	}

	id = atoi(argv[1]);
	if (argc == 3)
		config = argv[2];

	start_proposer(config, id);
	
	return 0;
}
