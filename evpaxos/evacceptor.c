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
#include "peers.h"
#include "acceptor.h"
#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <event2/event.h>


struct evacceptor
{
	struct peers* peers;
	struct acceptor* state;
	struct event* timer_ev;
	struct timeval timer_tv;
};

/**
 * Sends a Paxos message using the given peer's buffer.
 *
 * @param p A pointer to the peer structure representing the connection.
 * @param arg A pointer to the Paxos message to send.
 */
static void peer_send_paxos_message(struct peer* p, void* arg)
{
	send_paxos_message(peer_get_buffer(p), arg);
}

/*
	Received a prepare request (phase 1a).
*/

/**
 * Handles a received prepare request (phase 1a) from a peer.
 *
 * @param p A pointer to the peer structure representing the connection.
 * @param msg A pointer to the received Paxos message.
 * @param arg A pointer to the evacceptor structure.
 */
static void evacceptor_handle_prepare(struct peer* p, paxos_message* msg, void* arg)
{
	paxos_message out;
	paxos_log_debug("EVACCEPTOR --> Message Info: %x %x %x %x ", msg->msg_info[0], msg->msg_info[1], msg->msg_info[2], msg->msg_info[3]);
	paxos_prepare* prepare = &msg->u.prepare;
	struct evacceptor* a = (struct evacceptor*)arg;
	paxos_log_debug("Handle prepare for iid %d ballot %d", prepare->iid, prepare->ballot);
	if (acceptor_receive_prepare(a->state, prepare, &out) != 0) {
		paxos_log_debug("EVACCEPTOR --> Sending Message Info: %x %x %x %x ", msg->msg_info[0], msg->msg_info[1], msg->msg_info[2], msg->msg_info[3]);
		send_paxos_message(peer_get_buffer(p), &out);
		paxos_message_destroy(&out);
	}
}

/*
	Received a accept request (phase 2a).
*/

/**
 * Handles a received accept request (phase 2a) from a peer.
 *
 * @param p A pointer to the peer structure representing the connection.
 * @param msg A pointer to the received Paxos message.
 * @param arg A pointer to the evacceptor structure.
 */
static void evacceptor_handle_accept(struct peer* p, paxos_message* msg, void* arg)
{	
	paxos_message out;
	paxos_accept* accept = &msg->u.accept;
	paxos_log_debug("EVACCEPTOR --> (Handle Accept) accept Message Info : %x %x %x %x", msg->msg_info[0], msg->msg_info[1], msg->msg_info[2], msg->msg_info[3]);
	struct evacceptor* a = (struct evacceptor*)arg;
	paxos_log_debug("Handle accept for iid %d bal %d", 
		accept->iid, accept->ballot);
	if (acceptor_receive_accept(a->state, accept, &out) != 0) {
		if (out.type == PAXOS_ACCEPTED) {
			peers_foreach_client(a->peers, peer_send_paxos_message, &out);
		} else if (out.type == PAXOS_PREEMPTED) {
			send_paxos_message(peer_get_buffer(p), &out);
		}
		paxos_log_debug("EVACCEPTOR --> (Handle Accept) out Message Info: %x %x %x %x", out.msg_info[0], out.msg_info[1], out.msg_info[2], out.msg_info[3]);
		paxos_message_destroy(&out);
	}
}

/**
 * Handles a received repeat request from a peer, resending accepted values
 * for a range of instance IDs.
 *
 * @param p A pointer to the peer structure representing the connection.
 * @param msg A pointer to the received Paxos message.
 * @param arg A pointer to the evacceptor structure.
 */
static void evacceptor_handle_repeat(struct peer* p, paxos_message* msg, void* arg)
{
	iid_t iid;
	paxos_accepted accepted;
	memset(&accepted, 0, sizeof(accepted));
	paxos_repeat* repeat = &msg->u.repeat;
	struct evacceptor* a = (struct evacceptor*)arg;
	paxos_log_debug("EVACCEPTOR --> (Handle Repeat) repeat Message Info: %x %x %x %x", msg->msg_info[0], msg->msg_info[1], msg->msg_info[2], msg->msg_info[3]);
	paxos_log_debug("Handle repeat for iids %d-%d", repeat->from, repeat->to);
	for (iid = repeat->from; iid <= repeat->to; ++iid) {
		if (acceptor_receive_repeat(a->state, iid, &accepted)) {
			send_paxos_accepted(peer_get_buffer(p), &accepted);
			paxos_accepted_destroy(&accepted);
		}
	}
}

/**
 * Handles a received trim request from a peer, updating the acceptor's state
 * to discard values below the specified instance ID.
 *
 * @param p A pointer to the peer structure representing the connection.
 * @param msg A pointer to the received Paxos message.
 * @param arg A pointer to the evacceptor structure.
 */
static void evacceptor_handle_trim(struct peer* p, paxos_message* msg, void* arg)
{
	paxos_trim* trim = &msg->u.trim;
	struct evacceptor* a = (struct evacceptor*)arg;
	acceptor_receive_trim(a->state, trim);
}

/**
 * Sends the current state of the acceptor to all connected clients (peers).
 *
 * @param fd The file descriptor associated with the event.
 * @param ev The event that occurred.
 * @param arg A pointer to the evacceptor structure.
 */
static void send_acceptor_state(int fd, short ev, void* arg)
{
	struct evacceptor* a = (struct evacceptor*)arg;
	paxos_message msg = {.type = PAXOS_ACCEPTOR_STATE};
	paxos_log_debug("EVACCEPTOR --> (Send State) Message Info: %x %x %x %x", msg.msg_info[0], msg.msg_info[1], msg.msg_info[2], msg.msg_info[3]);
	acceptor_set_current_state(a->state, &msg.u.state);
	peers_foreach_client(a->peers, peer_send_paxos_message, &msg);
	event_add(a->timer_ev, &a->timer_tv);
}

/**
 * Initializes an evacceptor structure with the specified acceptor ID, evpaxos
 * configuration, and peers structure. Subscribes the evacceptor to receive
 * messages from peers and sets up a timer to periodically send the acceptor's
 * state to connected peers.
 *
 * @param id The ID of the acceptor.
 * @param c A pointer to the evpaxos configuration.
 * @param p A pointer to the peers structure representing connected peers.
 * @return A pointer to the initialized evacceptor structure, or NULL on failure.
 */
struct evacceptor* evacceptor_init_internal(int id, struct evpaxos_config* c, struct peers* p)
{
	// Allocate memory for the evacceptor structure
	struct evacceptor* acceptor;
	acceptor = calloc(1, sizeof(struct evacceptor));

	// Create a new acceptor state associated with the specified ID
	acceptor->state = acceptor_new(id);
	acceptor->peers = p;
	
	// Subscribe the acceptor to handle various types of Paxos messages
	peers_subscribe(p, PAXOS_PREPARE, evacceptor_handle_prepare, acceptor);
	peers_subscribe(p, PAXOS_ACCEPT, evacceptor_handle_accept, acceptor);
	peers_subscribe(p, PAXOS_REPEAT, evacceptor_handle_repeat, acceptor);
	peers_subscribe(p, PAXOS_TRIM, evacceptor_handle_trim, acceptor);
	
	// Obtain the event base from the peers structure
	struct event_base* base = peers_get_event_base(p);
	// Create an event timer for sending the acceptor state periodically
	acceptor->timer_ev = evtimer_new(base, send_acceptor_state, acceptor);
	// Set the timer interval to 1 second
	acceptor->timer_tv = (struct timeval){2, 0};
	// Add the timer event to the event loop
	event_add(acceptor->timer_ev, &acceptor->timer_tv);

	return acceptor;
}

/**
 * Initializes an evacceptor structure with the specified acceptor ID, configuration
 * file path, and event base. Reads the evpaxos configuration from the specified
 * file, sets up a peers structure, and starts listening on the acceptor's port.
 *
 * @param id The ID of the acceptor.
 * @param config_file The path to the configuration file.
 * @param base A pointer to the event base associated with the evacceptor.
 * @return A pointer to the initialized evacceptor structure, or NULL on failure.
 */
struct evacceptor* evacceptor_init(int id, const char* config_file, struct event_base* base)
{
	// Read the evpaxos configuration from the specified file.
	struct evpaxos_config* config = evpaxos_config_read(config_file);
	if (config  == NULL)
		return NULL;
	
	// Get the total number of acceptors from the configuration
	int acceptor_count = evpaxos_acceptor_count(config);
	// Check if the provided acceptor ID is within valid range
	if (id < 0 || id >= acceptor_count) {
		paxos_log_error("Invalid acceptor id: %d.", id);
		paxos_log_error("Should be between 0 and %d", acceptor_count);
		evpaxos_config_free(config);
		return NULL;
	}

	// Create a new peers structure using the provided event base and configuration
	struct peers* peers = peers_new(base, config);
	// Get the port on which the acceptor should listen from the configuration
	int port = evpaxos_acceptor_listen_port(config, id);
	// Start listening for connections on the specified port
	if (peers_listen(peers, port) == 0)
		return NULL;
	// Initialize the evacceptor's internal components and return the pointer
	struct evacceptor* acceptor = evacceptor_init_internal(id, config, peers);
	evpaxos_config_free(config);
	return acceptor;
}

/**
 * Frees resources associated with the internal components of the evacceptor structure,
 * including event timers and the acceptor's state.
 *
 * @param a A pointer to the evacceptor structure to be freed.
 */
void evacceptor_free_internal(struct evacceptor* a)
{
	event_free(a->timer_ev);
	acceptor_free(a->state);
	free(a);
}

/**
 * Frees resources associated with the evacceptor structure, including the peers
 * structure and internal components. This function should be used to properly
 * clean up the memory allocated for an evacceptor instance.
 *
 * @param a A pointer to the evacceptor structure to be freed.
 */
void evacceptor_free(struct evacceptor* a)
{
	peers_free(a->peers);
	evacceptor_free_internal(a);
}
