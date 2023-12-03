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
#include <string.h>
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
	int    subordinates;
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

static void evacceptor_fwd_promise(struct peer* p, paxos_message* msg, void* arg)
{
	int srcid = -1;

	if (msg->type == PAXOS_PROMISE)
	{
		srcid = get_srcid_promise_and_adjust(&(msg->u.promise), ((struct evacceptor*)arg)->state);
	}

	if (srcid >= 0)
	{
		struct peer* srcpeer = peer_get_acceptor(p,srcid);

		if (srcpeer != NULL)
			send_paxos_message(peer_get_buffer(srcpeer), msg);
	}
}

static void evacceptor_fwd_preempted(struct peer* p, paxos_message* msg, void* arg)
{
	int srcid = -1;

	if (msg->type == PAXOS_PREEMPTED)
	{
		srcid = get_srcid_preempted(&(msg->u.preempted), ((struct evacceptor*)arg)->state);
	}

	if (srcid >= 0)
	{
		struct peer* srcpeer = peer_get_acceptor(p, srcid);

		if (srcpeer != NULL)
			send_paxos_message(peer_get_buffer(srcpeer), msg);
	}
}


static void evacceptor_fwd_accepted(struct peer* p, paxos_message* msg, void* arg)
{
	int srcid = -1;

	if (msg->type == PAXOS_ACCEPTED)
	{
		srcid = get_srcid_accepted(&(msg->u.accepted), ((struct evacceptor*)arg)->state);
	}

	if (srcid >= 0)
	{
		struct peer* srcpeer = peer_get_acceptor(p, srcid);

		if (srcpeer != NULL)
			send_paxos_message(peer_get_buffer(srcpeer), msg);
	}
}

/**
 * Received a prepare request (phase 1a).
 * 
 * Handles a received prepare request (phase 1a) from a peer.
 *
 * @param p A pointer to the peer structure representing the connection.
 * @param msg A pointer to the received Paxos message.
 * @param arg A pointer to the evacceptor structure.
 */
static void evacceptor_handle_prepare(struct peer* p, paxos_message* msg, void* arg)
{
	paxos_message out;
	// paxos_log_debug("EVACCEPTOR --> Message Info: %x %x %x %x ", msg->msg_info[0], msg->msg_info[1], msg->msg_info[2], msg->msg_info[3]);
	paxos_prepare* prepare = &msg->u.prepare;
	struct evacceptor* a = (struct evacceptor*)arg;
	paxos_log_debug("Acceptor %u Handle prepare for iid %u ballot %u",get_aid(a->state), prepare->iid, prepare->ballot);

	uint32_t originalsrc = prepare->src;
	prepare->src = get_aid(a->state);
	peers_foreach_down_acceptor(a->peers, peer_send_paxos_message, msg);
	prepare->src = originalsrc;

	if (acceptor_receive_prepare(prepare->src,a->state, prepare, &out) != 0) {
		// paxos_log_debug("EVACCEPTOR --> Sending Message Info: %x %x %x %x ", msg->msg_info[0], msg->msg_info[1], msg->msg_info[2], msg->msg_info[3]);
		send_paxos_message(peer_get_buffer(p), &out);
		paxos_message_destroy(&out);
	}
}

/**
 * Received a accept request (phase 2a).
 * 
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
	// paxos_log_debug("EVACCEPTOR --> (Handle Accept) accept Message Info : %x %x %x %x", msg->msg_info[0], msg->msg_info[1], msg->msg_info[2], msg->msg_info[3]);
	struct evacceptor* a = (struct evacceptor*) arg;
	paxos_log_debug("Acceptor %u Handle accept for iid %u bal %u", get_aid(a->state),accept->iid, accept->ballot);
	uint32_t originalsrc = accept->src;
	peers_foreach_down_acceptor(a->peers, peer_send_paxos_message, msg);
	accept->src = originalsrc;

	if (acceptor_receive_accept(a->state, accept, &out) != 0) {
		if (out.type == PAXOS_ACCEPTED) {
			peers_foreach_client(a->peers, peer_send_paxos_message, &out);
		} else if (out.type == PAXOS_PREEMPTED) {
			send_paxos_message(peer_get_buffer(p), &out);
		}

		// paxos_log_debug("EVACCEPTOR --> (Handle Accept) out Message Info: %x %x %x %x", out.msg_info[0], out.msg_info[1], out.msg_info[2], out.msg_info[3]);
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
	// paxos_log_debug("EVACCEPTOR --> (Handle Repeat) repeat Message Info: %x %x %x %x", msg->msg_info[0], msg->msg_info[1], msg->msg_info[2], msg->msg_info[3]);
	paxos_log_debug("Acceptor %u Handle repeat for iids %u-%u", get_aid(a->state), repeat->from, repeat->to);
//	peers_foreach_down_acceptor(a->peers, peer_send_paxos_message, msg);

	for (iid = repeat->from; iid <= repeat->to; ++iid) {
		//paxos_log_debug("processing iid %ld", iid);

		if (acceptor_receive_repeat(a->state, iid, &accepted)) {
			//paxos_log_debug("acceptor receive repeat %ld", iid);
			send_paxos_accepted(peer_get_buffer(p), &accepted);
			//paxos_log_debug("Repeat sent.");
			paxos_accepted_destroy(&accepted);
		}
		// else
			// paxos_log_debug("Acceptor did not receive repeat %ld", iid);
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
	peers_foreach_down_acceptor(a->peers, peer_send_paxos_message, msg);
	acceptor_receive_trim(a->state, trim);
}

/**
 * Sends the current state of the acceptor to all connected clients (peers).
 *
 * @param fd The file descriptor associated with the event.
 * @param ev The event that occurred.
 * @param arg A pointer to the evacceptor structure.
 */
unsigned long prevmsg = 0;
unsigned long etprev = 0;

static void send_acceptor_state(int fd, short ev, void* arg)
{
	struct evacceptor* a = (struct evacceptor*)arg;

	int off;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	unsigned long nmsgnew = getcnt();
	unsigned long nsec = tv.tv_sec - etprev;
	unsigned long tsec = tv.tv_sec;
	unsigned long diffmsg = nmsgnew - prevmsg;

	if (nsec != 0)
	{
		struct evpaxos_config* c = getconfigfrompeers(a->peers);
		int nreplicas = c->acceptors_count;
		char Buff[1024]; memset(Buff, 0, sizeof(Buff));
		off = strftime(Buff, sizeof(Buff), "%d %b %H:%M:%S;", localtime(&tv.tv_sec));
		sprintf(Buff + off, "%d;%ld;%ld;%d;%ld\n", getpid(), nmsgnew, nsec, nreplicas, diffmsg/nsec );
		etprev = tsec;
		prevmsg = nmsgnew;
		FILE* pf;
		pf = fopen("msgstat.csv", "a+");
		fputs(Buff, pf);
		fflush(pf);
		fclose(pf);
	}
	else
	{
//		sprintf(Buff + off, "pid;%d;msg;%ld;timediff;%ld;msgpersec;%ld\n", getpid(), nmsgnew, nsec, "N/A");
	}

	event_add(a->timer_ev, &a->timer_tv);
	return; // 14.11.2023

	paxos_message msg = {.type = PAXOS_ACCEPTOR_STATE};
	// paxos_log_debug("EVACCEPTOR --> (Send State) Message Info: %x %x %x %x", msg.msg_info[0], msg.msg_info[1], msg.msg_info[2], msg.msg_info[3]);
	peers_foreach_down_acceptor(a->peers, peer_send_paxos_message, (void*)&msg);
	acceptor_set_current_state(a->state, &msg.u.state);
	peers_foreach_client(a->peers, peer_send_paxos_message, &msg);


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
	
	int acceptor_count = c->acceptors_count;
	int* scanned = malloc(sizeof(int) * acceptor_count);
	for (int i = 0; i < acceptor_count; i++) scanned[i] = -1;
	int* toscan = malloc(sizeof(int) * acceptor_count);
	for (int i = 0; i < acceptor_count; i++) toscan[i] = -1;
	if (c->acceptors[id].groupid != c->acceptors[id].parentid) 	toscan[0] = c->acceptors[id].groupid;
	int ncnt = 0;

	while (toscan[0] != -1)
	{
		
		int grp = toscan[0]; 
//		paxos_log_debug("Acceptor %d scanning group %d", id, grp);
		for (int j = 0; j < acceptor_count - 1; j++) toscan[j] = toscan[j + 1]; toscan[acceptor_count - 1] = -1;
		for (int j = 0; j < acceptor_count; j++) { if (scanned[j] == grp) break; else if (scanned[j] == -1) { scanned[j] = grp; break; } };

		for (int i = 0; i < acceptor_count; i++)
		{
			if (c->acceptors[i].groupid == grp 
				 && c->acceptors[i].groupid == c->acceptors[id].parentid )
			{
				ncnt++;
			}
			else
				if (c->acceptors[i].parentid == grp /*  && c->acceptors[i].parentid != c->acceptors[i].groupid*/ )
				{
					ncnt++;
					{
						int fnd = 0;
						for (int k = 0; k < acceptor_count; k++) if (scanned[k] == c->acceptors[i].groupid) { fnd = 1; break; }
						if (fnd == 0)
						{
							for (int k = 0; k < acceptor_count; k++) if (toscan[k] == c->acceptors[i].groupid) { fnd = 1; break; }
						}
						if (fnd == 0)
						{
							for (int l = 0; l < acceptor_count; l++) { if (toscan[l] == c->acceptors[i].groupid) break; else if (toscan[l] == -1) { toscan[l] = c->acceptors[i].groupid; break; } };
						}
					}

				}
		}

	}
	free(scanned);
	free(toscan);
	acceptor->subordinates = ncnt;
	setsubordinates(acceptor->state, ncnt);
//	paxos_log_debug("Acceptor %d subordinates %d", id, ncnt);


	// Subscribe the acceptor to handle various types of Paxos messages
	peers_subscribe(p, PAXOS_PREPARE, evacceptor_handle_prepare, acceptor);
	peers_subscribe(p, PAXOS_ACCEPT, evacceptor_handle_accept, acceptor);
	peers_subscribe(p, PAXOS_REPEAT, evacceptor_handle_repeat, acceptor);
	peers_subscribe(p, PAXOS_TRIM, evacceptor_handle_trim, acceptor);
	peers_subscribe(p, PAXOS_PROMISE, evacceptor_fwd_promise, acceptor);
	peers_subscribe(p, PAXOS_ACCEPTED, evacceptor_fwd_accepted, acceptor);
	peers_subscribe(p, PAXOS_PREEMPTED, evacceptor_fwd_preempted, acceptor);


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
	paxos_log_debug("Acceptor %d entering init");

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
