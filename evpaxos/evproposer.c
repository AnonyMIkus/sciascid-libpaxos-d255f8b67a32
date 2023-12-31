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
#include "message.h"
#include "proposer.h"
#include <string.h>
#include <stdlib.h>
#include <event2/event.h>

struct evproposer
{
	int id;
	int preexec_window;
	struct proposer* state;
	struct peers* peers;
	struct timeval tv;
	struct event* timeout_ev;
};

/**
 * Sends a paxos_prepare message to the specified peer.
 *
 * @param p Pointer to the peer structure to which the paxos_prepare message will be sent.
 * @param arg A pointer to the argument to be passed to the paxos_prepare message.
 */
static void peer_send_prepare(struct peer* p, void* arg)
{
	//getcnt();
	send_paxos_prepare(peer_get_buffer(p), arg);
}


/**
 * Sends a paxos_accept message to the specified peer.
 *
 * @param p Pointer to the peer structure to which the paxos_accept message will be sent.
 * @param arg A pointer to the argument to be passed to the paxos_accept message.
 */
static void peer_send_accept(struct peer* p, void* arg)
{
	//getcnt();
	send_paxos_accept(peer_get_buffer(p), arg);
}

/**
 * Executes the preexecution step for the proposer, preparing instances for acceptance.
 *
 * @param p Pointer to the evproposer structure.
 */
static void proposer_preexecute(struct evproposer* p)
{
	if (proposer_no_values(p->state))
		return;

	int i;
	paxos_prepare pr;
	int count = p->preexec_window - proposer_prepared_count(p->state);

	if (count <= 0)
		return;

	for (i = 0; i < count; i++) {
		proposer_prepare(p->state, &pr);
		peers_foreach_acceptor(p->peers, peer_send_prepare, &pr);
	}

	// paxos_log_debug("Opened %d new instances", count);
}

/**
 * Attempts to send acceptance messages to the acceptors for the instances that can be accepted.
 *
 * @param p Pointer to the evproposer structure.
 */
static void try_accept(struct evproposer* p)
{
	paxos_accept accept;

	while (proposer_accept(p->state, &accept))
		peers_foreach_acceptor(p->peers, peer_send_accept, &accept);

	proposer_preexecute(p);
}

/**
 * Handles the promise message received from an acceptor.
 *
 * @param p Pointer to the peer structure.
 * @param msg Pointer to the paxos_message received.
 * @param arg Pointer to the evproposer structure.
 */
static void evproposer_handle_promise(struct peer* p, paxos_message* msg, void* arg)
{
	struct evproposer* proposer = arg;
	paxos_prepare prepare;
	paxos_promise* pro = &msg->u.promise;
	int preempted = proposer_receive_promise(proposer->state, pro, &prepare);

	if (preempted)
		peers_foreach_acceptor(proposer->peers, peer_send_prepare, &prepare);

	paxos_log_debug("Proposer %u handling promise from %u trying to accept", get_prid(proposer->state), pro->aids[0]);
	try_accept(proposer);
}

/**
 * Handles the accepted message received from an acceptor.
 *
 * @param p Pointer to the peer structure.
 * @param msg Pointer to the paxos_message received.
 * @param arg Pointer to the evproposer structure.
 */
static void evproposer_handle_accepted(struct peer* p, paxos_message* msg, void* arg)
{
	struct evproposer* proposer = arg;
	paxos_accepted* acc = &msg->u.accepted;
	if (proposer_receive_accepted(proposer->state, acc))
		try_accept(proposer);
}

/**
 * Handle a preempted message received by an event proposer.
 *
 * @param p A pointer to the peer from which the message was received.
 * @param msg A pointer to the Paxos message received.
 * @param arg A pointer to the event proposer associated with this handler.
 */
static void evproposer_handle_preempted(struct peer* p, paxos_message* msg, void* arg)
{
	struct evproposer* proposer = arg;
	paxos_prepare prepare;
	int preempted = proposer_receive_preempted(proposer->state, &msg->u.preempted, &prepare);

	if (preempted) {
		peers_foreach_acceptor(proposer->peers, peer_send_prepare, &prepare);
		try_accept(proposer);
	}
}

/**
 * Handle a 


 message received by an event proposer.
 *
 * @param p A pointer to the peer from which the message was received.
 * @param msg A pointer to the Paxos message received.
 * @param arg A pointer to the event proposer associated with this handler.
 */
static void evproposer_handle_client_value(struct peer* p, paxos_message* msg, void* arg)
{
	struct evproposer* proposer = arg;
	struct paxos_client_value* v = &msg->u.client_value;
	paxos_log_debug("Proposer %u client value request", get_prid(proposer->state));
	proposer_propose(proposer->state, v->value.paxos_value_val, v->value.paxos_value_len);
	try_accept(proposer);
	paxos_log_debug("Proposer %u client value request completed", get_prid(proposer->state));
}

/**
 * Handles the acceptor_state message received from an acceptor.
 *
 * @param p Pointer to the peer structure.
 * @param msg Pointer to the paxos_message received.
 * @param arg Pointer to the evproposer structure.
 */
static void evproposer_handle_acceptor_state(struct peer* p, paxos_message* msg, void* arg)
{
	struct evproposer* proposer = arg;
	struct paxos_acceptor_state* acc_state = &msg->u.state;
	proposer_receive_acceptor_state(proposer->state, acc_state);
}

/**
 * Checks for timeouts in the proposer and triggers corresponding actions.
 *
 * @param fd File descriptor.
 * @param event Type of event.
 * @param arg Pointer to the evproposer structure.
 */
static void evproposer_check_timeouts(evutil_socket_t fd, short event, void *arg)
{
	return; // 11:26 14.11.2023

	struct evproposer* p = arg;
	struct timeout_iterator* iter = proposer_timeout_iterator(p->state);

	paxos_prepare pr;	
	while (timeout_iterator_prepare(iter, &pr)) {
		// paxos_log_info("Instance %d timed out in phase 1.", pr.iid);
		peers_foreach_acceptor(p->peers, peer_send_prepare, &pr);
	}
	
	paxos_accept ar;
	while (timeout_iterator_accept(iter, &ar)) {
		// paxos_log_info("Instance %d timed out in phase 2.", ar.iid);
		peers_foreach_acceptor(p->peers, peer_send_accept, &ar);
	}
	
	timeout_iterator_free(iter);
	event_add(p->timeout_ev, &p->tv);
}

/**
 * Executes the preexecution step for the proposer once.
 *
 * @param fd File descriptor.
 * @param event Type of event.
 * @param arg Pointer to the evproposer structure.
 */
static void evproposer_preexec_once(evutil_socket_t fd, short event, void *arg)
{
	struct evproposer* p = arg;
	proposer_preexecute(p);
	paxos_log_debug("Proposer %u: Preexec",p->id);
}

/**
 * Initializes the evproposer structure internally.
 *
 * @param id Proposer's ID.
 * @param c Pointer to the evpaxos_config structure.
 * @param peers Pointer to the peers structure.
 * @return Pointer to the initialized evproposer structure.
 */
struct evproposer* evproposer_init_internal(int id, struct evpaxos_config* c, struct peers* peers)
{
	struct evproposer* p;
	int acceptor_count = evpaxos_acceptor_count(c);
	p = malloc(sizeof(struct evproposer));
	p->id = id;
	p->preexec_window = paxos_config.proposer_preexec_window;

	peers_subscribe(peers, PAXOS_PROMISE, evproposer_handle_promise, p);
	peers_subscribe(peers, PAXOS_ACCEPTED, evproposer_handle_accepted, p);
	peers_subscribe(peers, PAXOS_PREEMPTED, evproposer_handle_preempted, p);
	peers_subscribe(peers, PAXOS_CLIENT_VALUE, evproposer_handle_client_value, p);
	peers_subscribe(peers, PAXOS_ACCEPTOR_STATE, evproposer_handle_acceptor_state, p);

	// Setup timeout
	struct event_base* base = peers_get_event_base(peers);
	p->tv.tv_sec = paxos_config.proposer_timeout;
	p->tv.tv_usec = 0;
	p->timeout_ev = evtimer_new(base, evproposer_check_timeouts, p);
	event_add(p->timeout_ev, &p->tv);
	p->state = proposer_new(p->id, acceptor_count);
	p->peers = peers;
	
	// Perform preexecution step using an event base timeout
	event_base_once(base, 0, EV_TIMEOUT, evproposer_preexec_once, p, NULL);
	return p;
}

/**
 * Initializes the evproposer structure.
 *
 * @param id Proposer's ID.
 * @param config_file Path to the configuration file.
 * @param base Pointer to the event_base structure.
 * @return Pointer to the initialized evproposer structure.
 */
struct evproposer* evproposer_init(int id, const char* config_file, struct event_base* base)
{
	struct evpaxos_config* config = evpaxos_config_read(config_file);

	if (config == NULL)
		return NULL;

	// Check id validity of proposer_id
	if (id < 0 || id >= MAX_N_OF_PROPOSERS) {
		paxos_log_error("Invalid proposer id: %d", id);
		return NULL;
	}
	
	struct peers* peers = peers_new(base, config);
	peers_connect_to_acceptors(peers, 0);
	int port = evpaxos_proposer_listen_port(config, id);
	int rv = peers_listen(peers, port);

	if (rv == 0)
		return NULL;

	struct evproposer* p = evproposer_init_internal(id, config, peers);
	evpaxos_config_free(config);
	return p;
}

/**
 * Frees internal resources of the evproposer structure.
 *
 * @param p Pointer to the evproposer structure.
 */
void evproposer_free_internal(struct evproposer* p)
{
	if (p != NULL) event_free(p->timeout_ev);
	if (p != NULL) proposer_free(p->state);
	if (p != NULL) free(p);
}

/**
 * Frees resources of the evproposer structure.
 *
 * @param p Pointer to the evproposer structure.
 */
void evproposer_free(struct evproposer* p)
{
	if (p != NULL) 
		peers_free(p->peers);
	if (p != NULL) 
		evproposer_free_internal(p);
}

/**
 * Sets the instance ID for the evproposer structure.
 *
 * @param p Pointer to the evproposer structure.
 * @param iid Instance ID to set.
 */
void evproposer_set_instance_id(struct evproposer* p, unsigned iid)
{
	if(p!=NULL)
		proposer_set_instance_id(p->state, iid);
}
