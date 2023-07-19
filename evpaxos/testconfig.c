#include "paxos.h"
#include "evpaxos.h"
#include <arpa/inet.h>
#include <netcx/shared/1.0/net/virtualaddress.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>

struct address
{
	char* addr;
	int port;
};

struct evpaxos_config
{
	int proposers_count;
	int acceptors_count;
	struct address proposers[MAX_N_OF_PROPOSERS];
	struct address acceptors[MAX_N_OF_PROPOSERS];
};

enum option_type
{
	option_boolean,
	option_integer,
	option_string,
	option_verbosity,
	option_backend,
	option_bytes
};

// int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
struct virtualAddressStruct {
	char hostbuffer[256];
	struct hostname *host_entry;
	hostname = gethostname(hostbuffer);
};