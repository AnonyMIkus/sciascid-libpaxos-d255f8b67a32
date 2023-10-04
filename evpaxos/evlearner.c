/*
 * Copyright (c) 2013-2014, University of Lugano
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


#include "evpaxos.h"
#include "learner.h"
#include "peers.h"
#include "message.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <event2/event.h>

struct evlearner
{
	struct learner* state;      /* The actual learner */
	deliver_function delfun;    /* Delivery callback */
	void* delarg;               /* The argument to the delivery callback */
	struct event* hole_timer;   /* Timer to check for holes */
	struct timeval tv;          /* Check for holes every tv units of time */
	struct peers* acceptors;    /* Connections to acceptors */
};


/**
 * This function sends a repeat message to a peer using the provided buffer and argument.
 * It is typically used to request the retransmission of Paxos messages that were missed.
 *
 * @param p A pointer to the peer to which the repeat message should be sent.
 * @param arg The argument to be included in the repeat message.
 */
static void peer_send_repeat(struct peer* p, void* arg)
{
	send_paxos_repeat(peer_get_buffer(p), arg);
}

/**
 * This function checks if there are message gaps in the received Paxos messages and requests
 * retransmissions for missing messages. It periodically sends repeat requests to peers
 * to fill gaps in the message sequence.
 *
 * @param fd The file descriptor (unused).
 * @param event The event flags (unused).
 * @param arg A pointer to the event-driven learner structure.
 */
static void evlearner_check_holes(evutil_socket_t fd, short event, void *arg)
{
	paxos_repeat msg;
	int chunks = 10;
	struct evlearner* l = arg;

	if (learner_has_holes(l->state, &msg.from, &msg.to)) {
		if ((msg.to - msg.from) > chunks)
			msg.to = msg.from + chunks;
		peers_foreach_acceptor(l->acceptors, peer_send_repeat, &msg);
	}

	event_add(l->hole_timer, &l->tv);
}

/**
 * This function delivers the next closed Paxos message to the application layer using the provided
 * delivery function. It iterates through the received Paxos messages and delivers them in sequence.
 *
 * @param l A pointer to the event-driven learner structure.
 */
static void evlearner_deliver_next_closed(struct evlearner* l)
{
	paxos_accepted deliver;
	memset(&deliver, 0, sizeof(paxos_accepted));

	while (learner_deliver_next(l->state, &deliver)) {
		l->delfun(deliver.iid, deliver.values[0].paxos_value_val, deliver.values[0].paxos_value_len, l->delarg);
		paxos_accepted_destroy(&deliver);
		memset(&deliver, 0, sizeof(paxos_accepted));
	}
}

/*
	Called when an accept_ack is received, the learner will update it's status
    for that instance and afterwards check if the instance is closed
*/

/**
 * This internal function initializes an event-driven learner with the provided configuration and peers.
 * It sets up the necessary event timers and subscriptions for Paxos messages.
 *
 * @param config The configuration for Paxos.
 * @param peers The peers with which the learner communicates.
 * @param f The delivery function for delivering Paxos messages to the application layer.
 * @param arg The argument to be passed to the delivery function.
 * @return A pointer to the initialized event-driven learner structure.
 */
static void evlearner_handle_accepted(struct peer* p, paxos_message* msg, void* arg)
{
	struct evlearner* l = arg;
	learner_receive_accepted(l->state, &msg->u.accepted);
	evlearner_deliver_next_closed(l);
}

/**
 * This internal function initializes an event-driven learner with the provided configuration and peers.
 * It sets up the necessary event timers and subscriptions for Paxos messages.
 *
 * @param config The configuration for Paxos.
 * @param peers The peers with which the learner communicates.
 * @param f The delivery function for delivering Paxos messages to the application layer.
 * @param arg The argument to be passed to the delivery function.
 * @return A pointer to the initialized event-driven learner structure.
 */
struct evlearner* evlearner_init_internal(struct evpaxos_config* config, struct peers* peers, deliver_function f, void* arg)
{
	int acceptor_count = evpaxos_acceptor_count(config);
	struct event_base* base = peers_get_event_base(peers);
	struct evlearner* learner = malloc(sizeof(struct evlearner));
	
	// Set up underlaying learner.
	learner->delfun = f;
	learner->delarg = arg;
	learner->state = learner_new(acceptor_count);
	learner->acceptors = peers;
	
	peers_subscribe(peers, PAXOS_ACCEPTED, evlearner_handle_accepted, learner);
	
	// Setup hole checking timer
	learner->tv.tv_sec = 0;
	learner->tv.tv_usec = 100000;
	learner->hole_timer = evtimer_new(base, evlearner_check_holes, learner);
	event_add(learner->hole_timer, &learner->tv);
	return learner;
}


/**
 * This function initializes an event-driven learner with the provided configuration file, delivery function,
 * and event base. It creates a configuration object, connects to acceptors, and sets up the learner.
 *
 * @param config_file The path to the configuration file for Paxos.
 * @param f The delivery function for delivering Paxos messages to the application layer.
 * @param arg The argument to be passed to the delivery function.
 * @param b The event base for event-driven operations.
 * @return A pointer to the initialized event-driven learner structure.
 */
struct evlearner* evlearner_init(const char* config_file, deliver_function f, void* arg, struct event_base* b)
{
	struct evpaxos_config* c = evpaxos_config_read(config_file);

	if (c == NULL) 
		return NULL;

	struct peers* peers = peers_new(b, c);
	peers_connect_to_acceptors(peers);
	struct evlearner* l = evlearner_init_internal(c, peers, f, arg);
	evpaxos_config_free(c);
	return l;
}

/**
 * This internal function frees the resources associated with an event-driven learner,
 * including the event timer for hole checking and the learner's state.
 *
 * @param l A pointer to the event-driven learner structure to be freed.
 */
void evlearner_free_internal(struct evlearner* l)
{
	event_free(l->hole_timer);
	learner_free(l->state);
	free(l);
}

/**
 * This function frees the resources associated with an event-driven learner, including
 * the acceptors and the internal resources. It calls the internal function to perform
 * the actual freeing of resources.
 *
 * @param l A pointer to the event-driven learner structure to be freed.
 */
void evlearner_free(struct evlearner* l)
{
	peers_free(l->acceptors);
	evlearner_free_internal(l);
}


/**
 * This function sets the instance ID for the event-driven learner's internal state. It allows
 * the learner to keep track of the current instance being processed.
 *
 * @param l A pointer to the event-driven learner structure.
 * @param iid The instance ID to be set.
 */
void evlearner_set_instance_id(struct evlearner* l, unsigned iid)
{
	learner_set_instance_id(l->state, iid);
}


/**
 * This function sends a trim message to all peers using the provided instance ID.
 * Trim messages are used to indicate that certain Paxos instances can be safely forgotten.
 *
 * @param p A pointer to the peer to which the trim message should be sent.
 * @param arg The argument containing the instance ID to be included in the trim message.
 */
static void peer_send_trim(struct peer* p, void* arg)
{
	send_paxos_trim(peer_get_buffer(p), arg);
}


/**
 * This function sends a trim message to all peers, indicating that a specific Paxos instance
 * with the given instance ID can be trimmed or forgotten.
 *
 * @param l A pointer to the event-driven learner structure.
 * @param iid The instance ID for which the trim message is sent.
 */
void evlearner_send_trim(struct evlearner* l, unsigned iid)
{
	paxos_trim trim = {iid};
	peers_foreach_acceptor(l->acceptors, peer_send_trim, &trim);
}

void evlearner_get_quorum()
{

}