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

struct option
{
	const char* name;
	void* value;
	enum option_type type;
};

struct option options[] =
{
	{ "verbosity", &paxos_config.verbosity, option_verbosity },
	{ "tcp-nodelay", &paxos_config.tcp_nodelay, option_boolean },
	{ "learner-catch-up", &paxos_config.learner_catch_up, option_boolean },
	{ "proposer-timeout", &paxos_config.proposer_timeout, option_integer },
	{ "proposer-preexec-window", &paxos_config.proposer_preexec_window, option_integer },
	{ "storage-backend", &paxos_config.storage_backend, option_backend },
	{ "acceptor-trash-files", &paxos_config.trash_files, option_boolean },
	{ "lmdb-sync", &paxos_config.lmdb_sync, option_boolean },
	{ "lmdb-env-path", &paxos_config.lmdb_env_path, option_string },
	{ "lmdb-mapsize", &paxos_config.lmdb_mapsize, option_bytes },
	{ 0 }
};

void init_config() {
	struct stat sb;
	FILE* f = NULL;
	char line[512];
	int linenumber = 1;
	struct evpaxos_config* c = NULL;

	if ((f = fopen(path, "r")) == NULL) {
		perror("fopen");
		goto failure;
	}

	if (stat(path, &sb) == -1) {
		perror("stat");
		goto failure;
	}

	if (!S_ISREG(sb.st_mode)) {
		paxos_log_error("Error: %s is not a regular file\n", path);
		goto failure;
	}

	c = malloc(sizeof(struct evpaxos_config));
	if (c == NULL) {
		perror("malloc");
		goto failure;
	}
	memset(c, 0, sizeof(struct evpaxos_config));

	while (fgets(line, sizeof(line), f) != NULL) {
		if (line[0] != '#' && line[0] != '\n') {
			if (parse_line(c, line) == 0) {
				paxos_log_error("Please, check line %d\n", linenumber);
				paxos_log_error("Error parsing config file %s\n", path);
				goto failure;
			}
		}
		linenumber++;
	}

	fclose(f);
	int count = c->acceptors_count + c->proposers_count;
	char listOfAddr[count];

	for (int i = 0; i < count; i++)
	{
		
	}
	return c;

failure:
	free(c);
	if (f != NULL) fclose(f);
	return NULL;
}