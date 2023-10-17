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


#include "evpaxos_internal.h"
#include "message.h"
#include <stdlib.h>
#include <pthread.h>

struct evpaxos_replica
{
	struct peers* peers;
	struct evlearner* learner;
	struct evproposer* proposer;
	struct evacceptor* acceptor;
	deliver_function deliver;
	void* arg;
};

struct evpaxos_parms
{
	int id;
	struct evpaxos_config* config;
	deliver_function f;
	void* arg;
	struct event_base* base;
	pthread_t* thread;
	pthread_mutex_t* tsync;
	pthread_mutex_t* pgs;
};

/**
 * This function allocates and initializes a structure to hold parameters for an
 * event-driven Paxos replica. It is used to pass these parameters to the replica's
 * initialization thread.
 *
 * @param id The unique identifier for the Paxos replica.
 * @param config A pointer to the Paxos configuration.
 * @param cb The delivery callback function for Paxos values.
 * @param arg An additional argument to be passed to the delivery callback function.
 * @param base The event base for the Paxos replica.
 * @param isync A pointer to the mutex for thread synchronization.
 * @param pgs A pointer to the mutex for Paxos message processing.
 * @return A pointer to the initialized parameter structure or NULL on failure.
 */
struct evpaxos_parms* evpaxos_alloc_parms(int id, struct evpaxos_config* config, deliver_function cb, void* arg, struct event_base* base, pthread_mutex_t* isync, pthread_mutex_t* pgs)
{
	struct evpaxos_parms* p = malloc(sizeof(struct evpaxos_parms));

	if (!p) 
		return p;

	p->id = id;
	p->config = config;
	p->f = cb;
	p->arg = arg;
	p->base = base;
	p->tsync = isync;
	p->pgs = pgs;
	return p;
}

/**
 * This function is responsible for delivering a Paxos value (a consensus decision)
 * to the Paxos replica. It sets the instance ID and invokes the user-defined delivery
 * callback function if one is registered.
 *
 * @param iid The instance ID of the delivered Paxos value.
 * @param value A pointer to the Paxos value being delivered.
 * @param size The size of the Paxos value.
 * @param arg A pointer to the Paxos replica structure.
 */
static void evpaxos_replica_deliver(unsigned iid, char* value, size_t size, void* arg)
{
	struct evpaxos_replica* r = arg;
	paxos_log_debug("In replica learner callback with proposer %lx",(unsigned long)(r->proposer));
	evproposer_set_instance_id(r->proposer, iid);
	paxos_log_debug("In replica learner callback proposer instance set");
	if (r->deliver)
		r->deliver(iid, value, size, r->arg);
	paxos_log_debug("Out replica learner callback");
}

/**
 * This function is used to start a new thread for initializing a Paxos replica. It sets up
 * the necessary parameters for replica initialization and invokes the `evpaxos_replica_init`
 * function in the new thread.
 *
 * @param inp A pointer to the input parameters for replica initialization.
 * @return NULL (thread exit)
 */
void* evpaxos_replica_init_thread_start(void* inp)
{
	paxos_log_debug("In New thread");
	struct evpaxos_parms* p = (struct evpaxos_parms*) inp;
	paxos_log_debug("Entering replica init");
	struct evpaxos_replica* r = evpaxos_replica_init(p->id, p->config, p->f, p->arg, p->base);
	paxos_log_debug("Exiting replica init");

	if (r == NULL)
		return NULL;

	pthread_mutex_lock(p->tsync);
	paxos_log_debug("Locked sync, exiting");
	pthread_mutex_destroy(p->tsync);	
	evpaxos_replica_free(r);
	pthread_exit(NULL);
	return NULL;
}

/**
 * This function initializes a Paxos replica in a new thread. It sets up the necessary parameters
 * and starts the thread by calling `pthread_create`. It also handles thread synchronization.
 *
 * @param inref A pointer to the thread reference variable.
 * @param insyncs A pointer to the thread synchronization mutex.
 * @param p A pointer to the parameters for replica initialization.
 * @return Returns 0 on success or an error code on failure.
 */
int evpaxos_replica_init_thread(void* inref, void* insyncs, struct evpaxos_parms* p)
{
	paxos_log_debug("Start threading process ...");
	pthread_t* ref = (pthread_t*)inref;
	pthread_mutex_t* syncs = (pthread_mutex_t*)insyncs;
	p->thread = ref;
	p->tsync = syncs;
	paxos_log_debug("Set up mutex.");
	// Mutex initializing.
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(p->tsync, &attr);
	pthread_mutexattr_destroy(&attr);
	pthread_mutex_lock(p->tsync);
	paxos_log_debug("Create Thread");
	int rc = pthread_create(ref, NULL, evpaxos_replica_init_thread_start, (void*) p);
	paxos_log_debug("Create Thread finished");
	fflush(stdout);
	return rc;
}

/**
 * This function initializes a Paxos replica by setting up the necessary components,
 * including peers, acceptor, proposer, and learner. It also handles port listening and
 * connects to acceptors.
 *
 * @param id The unique identifier for the Paxos replica.
 * @param c A pointer to the Paxos configuration.
 * @param f The delivery callback function for Paxos values.
 * @param arg An additional argument to be passed to the delivery callback function.
 * @param base The event base for the Paxos replica.
 * @return A pointer to the initialized Paxos replica structure or NULL on failure.
 */
struct evpaxos_replica* evpaxos_replica_init(int id, struct evpaxos_config* c, deliver_function f, void* arg, struct event_base* base)
{
	struct evpaxos_replica* r;
	struct evpaxos_config* config;

	r = malloc(sizeof(struct evpaxos_replica));
	config = c;
	paxos_log_debug("Initializing peers");
	r->peers = peers_new(base, config);
	paxos_log_debug("Connecting to acceptors");
	peers_connect_to_acceptors(r->peers, id);
	paxos_log_debug("Init own acceptor");
	r->acceptor = evacceptor_init_internal(id, config, r->peers);
	
	if ((config->acceptors[id].parentid) <= 0 && (config->acceptors[id].groupid == config->acceptors[id].parentid))
	{
		paxos_log_debug("Init own proposer");
		r->proposer = evproposer_init_internal(id, config, r->peers);
	}
	else
	{
		r->proposer = NULL;
		paxos_log_debug("Proposer have not initialized.");
	}

	paxos_log_debug("Init own learner");
	r->learner  = evlearner_init_internal(config, r->peers, evpaxos_replica_deliver, r);
	r->deliver = f;
	r->arg = arg;
	paxos_log_debug("Got id %d", id);
	paxos_log_debug("Getting listener port");
	int port = evpaxos_acceptor_listen_port(config, id);
	paxos_log_debug("Got port % d", port);

	if (peers_listen(r->peers, port) == 0) {
		paxos_log_debug("\nListen failed");
		evpaxos_config_free(config);
		evpaxos_replica_free(r);
		return NULL;
	}

	paxos_log_debug("Listener started");
	// evpaxos_config_free(config);
	return r;
}

/**
 * This function frees the resources associated with an event-driven Paxos replica,
 * including its learner, proposer, acceptor, peers, and the replica itself.
 *
 * @param r A pointer to the Paxos replica structure to be freed.
 */
void evpaxos_replica_free(struct evpaxos_replica* r)
{
	if (r->learner)
		evlearner_free_internal(r->learner);

	evproposer_free_internal(r->proposer);
	evacceptor_free_internal(r->acceptor);
	peers_free(r->peers);
	free(r);
}

/**
 * This function sets the instance ID for an event-driven Paxos replica, which is used
 * to identify the current Paxos instance during consensus rounds.
 *
 * @param r A pointer to the Paxos replica for which the instance ID is to be set.
 * @param iid The new instance ID to set.
 */
void evpaxos_replica_set_instance_id(struct evpaxos_replica* r, unsigned iid)
{
	if (r->learner)
		evlearner_set_instance_id(r->learner, iid);

	evproposer_set_instance_id(r->proposer, iid);
}

/**
 * This function sends a Paxos trim message with the specified instance ID to all
 * acceptors in the Paxos network using the provided Paxos replica.
 *
 * @param p A pointer to the Paxos replica responsible for sending the trim message.
 * @param arg 
 */
static void peer_send_trim(struct peer* p, void* arg)
{
	send_paxos_trim(peer_get_buffer(p), arg);
}

/**
 * This function sends a Paxos trim message with the specified instance ID to all
 * acceptors in the Paxos network using the provided Paxos replica.
 *
 * @param r A pointer to the Paxos replica responsible for sending the trim message.
 * @param iid The instance ID to be included in the trim message.
 */
void evpaxos_replica_send_trim(struct evpaxos_replica* r, unsigned iid)
{
	paxos_trim trim = {iid};
	peers_foreach_acceptor(r->peers, peer_send_trim, &trim);
}


/**
 * This function submits a Paxos value to the Paxos network through the Paxos replica.
 * It attempts to submit the value to connected acceptors.
 *
 * @param r A pointer to the Paxos replica responsible for submitting the value.
 * @param value A pointer to the Paxos value to be submitted.
 * @param size The size of the Paxos value.
 */
void evpaxos_replica_submit(struct evpaxos_replica* r, char* value, int size)
{
	int i;
	struct peer* p;

	for (i = 0; i < peers_count(r->peers); ++i) {
		p = peers_get_acceptor(r->peers, i);
		if (peer_connected(p)) {
			paxos_submit(peer_get_buffer(p), value, size);
			return;
		}
	}
}

/**
 * This function returns the count of peers (acceptors) that are connected to a
 * specific Paxos replica.
 *
 * @param r A pointer to the Paxos replica for which to count the connected peers.
 * @return The count of connected peers.
 */
int evpaxos_replica_count(struct evpaxos_replica* r)
{
	return peers_count(r->peers);
}
