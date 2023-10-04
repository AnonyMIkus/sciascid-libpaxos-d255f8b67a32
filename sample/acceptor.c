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

 /**
  * @brief Signal handler for handling SIGINT.
  *
  * This function is a signal handler for SIGINT (Ctrl+C) and is used to gracefully exit the acceptor.
  * It prints a message indicating the signal received and exits the event loop.
  *
  * @param sig The signal number (SIGINT).
  * @param ev The event flags (unused).
  * @param arg A pointer to the event base associated with the acceptor.
  */
static void handle_sigint(int sig, short ev, void* arg)
{
	struct event_base* base = arg;
	printf("Caught signal %d\n", sig);
	event_base_loopexit(base, NULL);
}

/**
 * This function initializes and starts an acceptor for the distributed system using Paxos.
 * It creates an event base, sets up signal handling for SIGINT, and initializes the acceptor.
 * The acceptor is responsible for handling incoming Paxos messages.
 *
 * @param id The unique identifier of the acceptor.
 * @param config The path to the configuration file for Paxos.
 */
static void start_acceptor(int id, const char* config)
{
	struct evacceptor* acc;
	struct event_base* base;
	struct event* sig;

	// Threadsafe event call.
	struct event_config* event_config = event_config_new();
	base = event_base_new_with_config(event_config);
	event_config_free(event_config);
	// Signal handling
	sig = evsignal_new(base, SIGINT, handle_sigint, base);
	evsignal_add(sig, NULL);
	acc = evacceptor_init(id, config, base);

	if (acc == NULL) {
		printf("Could not start the acceptor\n");
		return;
	}
	
	signal(SIGPIPE, SIG_IGN);
	event_base_dispatch(base);
	event_free(sig);
	evacceptor_free(acc);
	event_base_free(base);
}

int main(int argc, char const *argv[])
{
	int id;
	const char* config = "../paxos.conf";

	if (argc != 2 && argc != 3) {
		printf("Usage: %s id [path/to/paxos.conf]\n", argv[0]);
		exit(0);
	}

	id = atoi(argv[1]);
	if (argc >= 3)
		config = argv[2];

	start_acceptor(id, config);

	return 1;
}
