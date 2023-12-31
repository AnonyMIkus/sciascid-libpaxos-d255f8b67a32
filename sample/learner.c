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


struct client_value
{
	int client_id;
	struct timeval t;
	size_t size;
	char value[0];
};

/**
 * This function handles the SIGINT signal (typically generated by pressing Ctrl+C)
 * and initiates the termination of the event loop associated with the provided event base.
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
 * This function is called when a consensus message is delivered to the learner.
 * It prints the received message's timestamp, value, and size to the console.
 *
 * @param iid The instance ID of the consensus message.
 * @param value The consensus message value.
 * @param size The size of the consensus message.
 * @param arg User-defined argument (unused).
 */
static void deliver(unsigned iid, char* value, size_t size, void* arg)
{
	struct client_value* val = (struct client_value*)value;
	printf("%ld.%06ld [%.16s] %ld bytes\n", val->t.tv_sec, val->t.tv_usec,
		val->value, (long)val->size);
}

/**
 * This function initializes and starts an EvPaxos learner based on the provided configuration file.
 * It sets up an event loop for the learner to listen for incoming consensus messages and handles
 * signal interrupts for graceful termination.
 *
 * @param config The path to the configuration file for EvPaxos.
 */
static void start_learner(const char* config)
{
	struct event* sig;
	struct evlearner* lea;
	struct event_base* base;

	// Threadsafe event call.
	struct event_config* event_config = event_config_new();
	base = event_base_new_with_config(event_config);
	event_config_free(event_config);

	lea = evlearner_init(config, deliver, NULL, base);
	if (lea == NULL) {
		printf("Could not start the learner!\n");
		exit(1);
	}
	
	// Signal handling
	sig = evsignal_new(base, SIGINT, handle_sigint, base);
	evsignal_add(sig, NULL);
	signal(SIGPIPE, SIG_IGN);

	// Finish event handling.
	event_base_dispatch(base);
	event_free(sig);
	evlearner_free(lea);
	event_base_free(base);
}

int main(int argc, char const *argv[])
{
	const char* config = "../paxos.conf";
	if (argc != 1 && argc != 2) {
		printf("Usage: %s [path/to/paxos.conf]\n", argv[0]);
		exit(1);
	}
	if (argc == 2)
		config = argv[1];
	start_learner(config);
	return 0;
}
