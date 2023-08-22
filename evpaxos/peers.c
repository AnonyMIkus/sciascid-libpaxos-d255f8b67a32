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


#include "peers.h"
#include "message.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <netinet/tcp.h>

struct peer
{
	int id;
	int status;
	struct bufferevent* bev;
	struct event* reconnect_ev;
	struct sockaddr_in addr;
	struct peers* peers;
};

struct subscription
{
	paxos_message_type type;
	peer_cb callback;
	void* arg;
};

struct peers
{
	int peers_count, clients_count;
	struct peer** peers;   /* peers we connected to */
	struct peer** clients; /* peers we accepted connections from */
	struct evconnlistener* listener;
	struct event_base* base;
	struct evpaxos_config* config;
	int subs_count;
	struct subscription subs[32];
};

static struct timeval reconnect_timeout = { 2,0 };
static struct peer* make_peer(struct peers* p, int id, struct sockaddr_in* in);
static void free_peer(struct peer* p);
static void free_all_peers(struct peer** p, int count);
static void connect_peer(struct peer* p);
static void peers_connect(struct peers* p, int id, struct sockaddr_in* addr);
static void on_read(struct bufferevent* bev, void* arg);
static void on_peer_event(struct bufferevent* bev, short ev, void* arg);
static void on_client_event(struct bufferevent* bev, short events, void* arg);
static void on_connection_timeout(int fd, short ev, void* arg);
static void on_listener_error(struct evconnlistener* l, void* arg);
static void on_accept(struct evconnlistener* l, evutil_socket_t fd,
	struct sockaddr* addr, int socklen, void* arg);
static void socket_set_nodelay(int fd);

/**
 * Create and Initialize a New Peers Structure
 *
 * This function is responsible for creating a new instance of the 'peers' structure,
 * which is used to manage network-related information within a distributed application.
 * The 'peers' structure integrates with the libevent and msgpack libraries for event
 * handling and configuration data management.
 *
 * @param base A pointer to a libevent event_base object, representing the event loop
 *             used for managing asynchronous events such as I/O and timers.
 * @param config A pointer to a msgpack-configured evpaxos_config object containing
 *               configuration settings relevant to the operation of the program.
 * @return A pointer to the newly created and initialized peers structure.
 *         Returns NULL if memory allocation fails.
 */
struct peers* peers_new(struct event_base* base, struct evpaxos_config* config)
{
	struct peers* p = malloc(sizeof(struct peers));
	p->peers_count = 0;
	p->clients_count = 0;
	p->subs_count = 0;
	p->peers = NULL;
	p->clients = NULL;
	p->listener = NULL;
	p->base = base;
	p->config = config;
	return p;
}

/**
 * Free Resources of Peers Structure
 *
 * This function releases the resources associated with a 'peers' structure, including
 * lists of peers and clients, as well as any libevent listener instance. It also
 * deallocates the memory used by the 'peers' structure itself.
 *
 * @param p A pointer to the peers structure to be freed.
 */
void peers_free(struct peers* p)
{
	free_all_peers(p->peers, p->peers_count);
	free_all_peers(p->clients, p->clients_count);
	if (p->listener != NULL)
		evconnlistener_free(p->listener);
	free(p);
}

/**
 * Get the Count of Peers
 *
 * Returns the count of peers currently stored within the provided peers structure.
 *
 * @param p A pointer to the peers structure.
 * @return The count of peers in the structure.
 */
int peers_count(struct peers* p)
{
	return p->peers_count;
}

/**
 * Connect to a Peer
 *
 * Establishes a connection to a peer identified by the given ID and address.
 * This function sets up the necessary event handling, including callbacks for
 * reading, handling events, and connecting.
 *
 * @param p A pointer to the peers structure.
 * @param id The identifier of the peer.
 * @param addr A pointer to the sockaddr_in structure representing the peer's address.
 */
static void peers_connect(struct peers* p, int id, struct sockaddr_in* addr)
{
	p->peers = realloc(p->peers, sizeof(struct peer*) * (p->peers_count + 1));
	p->peers[p->peers_count] = make_peer(p, id, addr);

	struct peer* peer = p->peers[p->peers_count];
	bufferevent_setcb(peer->bev, on_read, NULL, on_peer_event, peer);
	peer->reconnect_ev = evtimer_new(p->base, on_connection_timeout, peer);
	connect_peer(peer);

	p->peers_count++;
}

/**
 * Connect to Acceptors
 *
 * Establishes connections to all acceptors based on the provided peers structure's configuration.
 * It iterates through acceptor addresses, connecting to each acceptor using peers_connect.
 *
 * @param p A pointer to the peers structure.
 */
void peers_connect_to_acceptors(struct peers* p)
{
	int i;
	for (i = 0; i < evpaxos_acceptor_count(p->config); i++) {
		struct sockaddr_in addr = evpaxos_acceptor_address(p->config, i);
		peers_connect(p, i, &addr);
	}
}

/**
 * Iterate Through Acceptors
 *
 * Executes the given callback function for each acceptor in the peers structure.
 *
 * @param p A pointer to the peers structure.
 * @param cb The callback function to execute for each acceptor.
 * @param arg An additional argument to pass to the callback function.
 */
void peers_foreach_acceptor(struct peers* p, peer_iter_cb cb, void* arg)
{
	int i;
	for (i = 0; i < p->peers_count; ++i)
		cb(p->peers[i], arg);
}

/**
 * Iterate Through Clients
 *
 * Executes the given callback function for each client in the peers structure.
 *
 * @param p A pointer to the peers structure.
 * @param cb The callback function to execute for each client.
 * @param arg An additional argument to pass to the callback function.
 */
void peers_foreach_client(struct peers* p, peer_iter_cb cb, void* arg)
{
	int i;
	for (i = 0; i < p->clients_count; ++i)
		cb(p->clients[i], arg);
}

/**
 * Get Acceptor by ID
 *
 * Retrieves the peer associated with the provided ID from the peers structure.
 *
 * @param p A pointer to the peers structure.
 * @param id The identifier of the peer.
 * @return A pointer to the peer structure, or NULL if not found.
 */
struct peer* peers_get_acceptor(struct peers* p, int id)
{
	int i;
	for (i = 0; p->peers_count; ++i)
		if (p->peers[i]->id == id)
			return p->peers[i];
	return NULL;
}

/**
 * Get Buffer Event from Peer
 *
 * Retrieves the buffer event associated with the provided peer.
 *
 * @param p A pointer to the peer structure.
 * @return A pointer to the bufferevent associated with the peer.
 */
struct bufferevent* peer_get_buffer(struct peer* p)
{
	return p->bev;
}

/**
 * Get ID of Peer
 *
 * Retrieves the identifier of the provided peer.
 *
 * @param p A pointer to the peer structure.
 * @return The identifier of the peer.
 */
int peer_get_id(struct peer* p)
{
	return p->id;
}

/**
 * Check Peer Connection Status
 *
 * Checks if the provided peer is connected based on its status.
 *
 * @param p A pointer to the peer structure.
 * @return 1 if connected, 0 if not.
 */
int peer_connected(struct peer* p)
{
	return p->status == BEV_EVENT_CONNECTED;
}

/**
 * Listen for Connections
 *
 * Sets up a listener to accept incoming connections on the given port.
 *
 * @param p A pointer to the peers structure.
 * @param port The port to listen on.
 * @return 1 if successful, 0 if failed.
 */
int peers_listen(struct peers* p, int port)
{
	struct sockaddr_in addr;
	unsigned flags = LEV_OPT_CLOSE_ON_EXEC
		| LEV_OPT_CLOSE_ON_FREE
		| LEV_OPT_REUSEABLE;

	/* listen on the given port at address 0.0.0.0 */
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(0);
	addr.sin_port = htons(port);

	p->listener = evconnlistener_new_bind(p->base, on_accept, p, flags, -1,
		(struct sockaddr*)&addr, sizeof(addr));
	if (p->listener == NULL) {
		paxos_log_error("Failed to bind on port %d", port);
		return 0;
	}
	evconnlistener_set_error_cb(p->listener, on_listener_error);
	paxos_log_info("Listening on port %d", port);
	return 1;
}

/**
 * Subscribe to Peer Messages
 *
 * Adds a subscription for a specific message type, associating a callback function
 * and an additional argument to the peers structure.
 *
 * @param p A pointer to the peers structure.
 * @param type The message type to subscribe to.
 * @param cb The callback function to execute when the message is received.
 * @param arg An additional argument to pass to the callback function.
 */
void peers_subscribe(struct peers* p, paxos_message_type type, peer_cb cb, void* arg)
{
	struct subscription* sub = &p->subs[p->subs_count];
	sub->type = type;
	sub->callback = cb;
	sub->arg = arg;
	p->subs_count++;
}


/**
 * Get Event Base of Peers Structure
 *
 * Retrieves the event base associated with the provided peers structure.
 *
 * @param p A pointer to the peers structure.
 * @return A pointer to the event base.
 */
struct event_base* 	peers_get_event_base(struct peers* p)
{
	return p->base;
}

/**
 * Dispatch Message to Subscriptions
 *
 * Dispatches a received paxos message to the appropriate subscription callbacks
 * based on the message type.
 *
 * @param p A pointer to the peer structure.
 * @param msg A pointer to the received paxos message.
 */
static void dispatch_message(struct peer* p, paxos_message* msg)
{
	int i;
	for (i = 0; i < p->peers->subs_count; ++i) {
		struct subscription* sub = &p->peers->subs[i];
		if (sub->type == msg->type)
			sub->callback(p, msg, sub->arg);
	}
}

/**
 * Callback for Reading Data from Peer
 *
 * Handles incoming data from the peer's buffer event by processing paxos messages
 * and dispatching them to the appropriate subscription callbacks.
 *
 * @param bev A pointer to the bufferevent associated with the peer.
 * @param arg A pointer to the peer structure.
 */
static void on_read(struct bufferevent* bev, void* arg)
{
	paxos_message msg;
	struct peer* p = (struct peer*)arg;
	paxos_log_debug("read event for peer with id %ld port %ld ip %lx ", p->id, p->addr.sin_port,p->addr.sin_addr.s_addr);

	struct evbuffer* in = bufferevent_get_input(bev);
	while (recv_paxos_message(in, &msg)) {
		dispatch_message(p, &msg);
		paxos_message_destroy(&msg);
	}
}

/**
 * Callback for Peer Event Handling
 *
 * Handles events occurring on the peer's buffer event, such as connection status changes,
 * errors, or the end of the connection.
 *
 * @param bev A pointer to the bufferevent associated with the peer.
 * @param ev The event flags indicating the type of event.
 * @param arg A pointer to the peer structure.
 */
static void on_peer_event(struct bufferevent* bev, short ev, void* arg)
{
	paxos_log_debug("peer event");

	struct peer* p = (struct peer*)arg;

	if (ev & BEV_EVENT_CONNECTED) {
		paxos_log_info("Connected to %s:%d",
			inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
		p->status = ev;
	}
	else if (ev & BEV_EVENT_ERROR || ev & BEV_EVENT_EOF) {
		struct event_base* base;
		int err = EVUTIL_SOCKET_ERROR();
		paxos_log_error("%s (%s:%d)", evutil_socket_error_to_string(err),
			inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
		base = bufferevent_get_base(p->bev);
		bufferevent_free(p->bev);
		p->bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
		bufferevent_setcb(p->bev, on_read, NULL, on_peer_event, p);
		event_add(p->reconnect_ev, &reconnect_timeout);
		p->status = ev;
	}
	else {
		paxos_log_error("Event %d not handled", ev);
	}
}


/**
 * Callback for Client Event Handling
 *
 * Handles events occurring on a client's buffer event, such as connection errors or the end of the connection.
 *
 * @param bev A pointer to the bufferevent associated with the client.
 * @param ev The event flags indicating the type of event.
 * @param arg A pointer to the peer structure.
 */
static void on_client_event(struct bufferevent* bev, short ev, void* arg)
{
	struct peer* p = (struct peer*)arg;
	if (ev & BEV_EVENT_EOF || ev & BEV_EVENT_ERROR) {
		int i;
		struct peer** clients = p->peers->clients;
		for (i = p->id; i < p->peers->clients_count - 1; ++i) {
			clients[i] = clients[i + 1];
			clients[i]->id = i;
		}
		p->peers->clients_count--;
		p->peers->clients = realloc(p->peers->clients,
			sizeof(struct peer*) * (p->peers->clients_count));
		free_peer(p);
	}
	else {
		paxos_log_error("Event %d not handled", ev);
	}
}

/**
 * Callback for Connection Timeout
 *
 * Handles the case when a connection attempt to a peer times out.
 *
 * @param fd The file descriptor associated with the connection attempt.
 * @param ev The event flags indicating the type of event.
 * @param arg A pointer to the peer structure.
 */
static void on_connection_timeout(int fd, short ev, void* arg)
{
	connect_peer((struct peer*)arg);
}


/**
 * Callback for Listener Error
 *
 * Handles errors occurring on the listener and initiates shutting down the event loop.
 *
 * @param l A pointer to the evconnlistener structure.
 * @param arg A pointer to the peers structure.
 */
static void on_listener_error(struct evconnlistener* l, void* arg)
{
	int err = EVUTIL_SOCKET_ERROR();
	struct event_base* base = evconnlistener_get_base(l);
	paxos_log_error("Listener error %d: %s. Shutting down event loop.", err,
		evutil_socket_error_to_string(err));
	event_base_loopexit(base, NULL);
}

/**
 * Callback for Accepting Connections
 *
 * Handles the event of accepting an incoming connection from a client.
 *
 * @param l A pointer to the evconnlistener structure.
 * @param fd The file descriptor associated with the accepted connection.
 * @param addr A pointer to the sockaddr structure representing the client's address.
 * @param socklen The length of the sockaddr structure.
 * @param arg A pointer to the peers structure.
 */
static void on_accept(struct evconnlistener* l, evutil_socket_t fd,
	struct sockaddr* addr, int socklen, void* arg)
{
	struct peer* peer;
	struct peers* peers = arg;

	peers->clients = realloc(peers->clients,
		sizeof(struct peer*) * (peers->clients_count + 1));
	peers->clients[peers->clients_count] =
		make_peer(peers, peers->clients_count, (struct sockaddr_in*)addr);

	peer = peers->clients[peers->clients_count];
	bufferevent_setfd(peer->bev, fd);
	bufferevent_setcb(peer->bev, on_read, NULL, on_client_event, peer);
	bufferevent_enable(peer->bev, EV_READ | EV_WRITE);
	socket_set_nodelay(fd);

	paxos_log_info("Accepted connection from %s:%d",
		inet_ntoa(((struct sockaddr_in*)addr)->sin_addr),
		ntohs(((struct sockaddr_in*)addr)->sin_port));

	peers->clients_count++;
}

/**
 * Connect Peer
 *
 * Initiates a connection attempt to the specified peer using the provided buffer event.
 * Enables read and write events, initiates the socket connection, and sets TCP_NODELAY.
 *
 * @param p A pointer to the peer structure.
 */
static void connect_peer(struct peer* p)
{
	bufferevent_enable(p->bev, EV_READ | EV_WRITE);
	bufferevent_socket_connect(p->bev,
		(struct sockaddr*)&p->addr, sizeof(p->addr));
	socket_set_nodelay(bufferevent_getfd(p->bev));
	paxos_log_info("Connect to %s:%d",
		inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
}


/**
 * Create Peer
 *
 * Creates a new peer structure and initializes its members with the provided values.
 * Initializes a buffer event for the peer's communication, sets up other fields, and returns the created peer.
 *
 * @param peers A pointer to the peers structure.
 * @param id The identifier of the peer.
 * @param addr A pointer to the sockaddr_in structure representing the peer's address.
 * @return A pointer to the newly created peer structure.
 */
static struct peer* make_peer(struct peers* peers, int id, struct sockaddr_in* addr)
{
	struct peer* p = malloc(sizeof(struct peer));
	p->id = id;
	p->addr = *addr;
	p->bev = bufferevent_socket_new(peers->base, -1, BEV_OPT_CLOSE_ON_FREE);
	p->peers = peers;
	p->reconnect_ev = NULL;
	p->status = BEV_EVENT_EOF;
	return p;
}

/**
 * Free All Peers
 *
 * Frees memory and resources associated with an array of peer pointers.
 * Calls free_peer for each peer and then deallocates the array itself if applicable.
 *
 * @param p An array of pointers to peer structures.
 * @param count The number of peers in the array.
 */
static void free_all_peers(struct peer** p, int count)
{
	int i;
	for (i = 0; i < count; i++)
		free_peer(p[i]);
	if (count > 0)
		free(p);
}

/**
 * Free Peer
 *
 * Releases resources and deallocates memory associated with the provided peer structure.
 * Frees the buffer event and, if applicable, the reconnect event.
 *
 * @param p A pointer to the peer structure.
 */
static void free_peer(struct peer* p)
{
	bufferevent_free(p->bev);
	if (p->reconnect_ev != NULL)
		event_free(p->reconnect_ev);
	free(p);
}

/**
 * Set TCP_NODELAY Socket Option
 *
 * Sets the TCP_NODELAY option on the specified socket file descriptor for reduced delay in data transmission.
 *
 * @param fd The file descriptor of the socket.
 */
static void socket_set_nodelay(int fd)
{
	int flag = paxos_config.tcp_nodelay;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
}
