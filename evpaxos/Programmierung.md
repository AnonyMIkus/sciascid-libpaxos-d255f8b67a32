peers

```c
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

static struct timeval reconnect_timeout = {2,0};
static struct peer* make_peer(struct peers* p, int id, struct sockaddr_in* in);
static void free_peer(struct peer* p);
static void free_all_peers(struct peer** p, int count);
static void connect_peer(struct peer* p);
static void peers_connect(struct peers* p, int id, struct sockaddr_in* addr);
static void on_read(struct bufferevent* bev, void* arg);
static void on_peer_event(struct bufferevent* bev, short ev, void *arg);
static void on_client_event(struct bufferevent* bev, short events, void *arg);
static void on_connection_timeout(int fd, short ev, void* arg);
static void on_listener_error(struct evconnlistener* l, void* arg);
static void on_accept(struct evconnlistener *l, evutil_socket_t fd, struct sockaddr* addr, int socklen, void *arg);
static void socket_set_nodelay(int fd);

struct peers* peers_new(struct event_base* base, struct evpaxos_config* config){}
void peers_free(struct peers* p){}
int peers_count(struct peers* p){}
void peers_connect_to_acceptors(struct peers* p){}
int peers_listen(struct peers* p, int port){}
void peers_subscribe(struct peers* p, paxos_message_type type, peer_cb cb, void* arg){}
void peers_foreach_acceptor(struct peers* p, peer_iter_cb cb, void* arg){}
void peers_foreach_client(struct peers* p, peer_iter_cb cb, void* arg){}
struct peer* peers_get_acceptor(struct peers* p, int id){}
struct event_base* peers_get_event_base(struct peers* p) {}
int peer_get_id(struct peer* p){}
struct bufferevent* peer_get_buffer(struct peer* p){}
int peer_connected(struct peer* p){}

static void dispatch_message(struct peer* p, paxos_message* msg){}
static void connect_peer(struct peer* p){}
static void peers_connect(struct peers* p, int id, struct sockaddr_in* addr){}
static void on_read(struct bufferevent* bev, void* arg){}
static void on_peer_event(struct bufferevent* bev, short ev, void *arg){}
static void on_client_event(struct bufferevent* bev, short ev, void *arg) {}
static void on_connection_timeout(int fd, short ev, void* arg){}
static void on_listener_error(struct evconnlistener* l, void* arg) {}
static void on_accept(struct evconnlistener *l, evutil_socket_t fd,	struct sockaddr* addr, int socklen, void *arg){}
static struct peer* make_peer(struct peers* peers, int id, struct sockaddr_in* addr) {}
static void free_all_peers(struct peer** p, int count){}
static void free_peer(struct peer* p){}
static void socket_set_nodelay(int fd){}
```