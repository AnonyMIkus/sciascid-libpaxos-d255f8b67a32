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


#include "paxos.h"
#include "evpaxos.h"
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>

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
	pthread_mutex_t* pgs;
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

struct virt_address
{
	char* virt_addr;
	char* virt_port;
};

struct network_sim
{
	struct virt_address virtual_addresses[MAX_N_OF_PROPOSERS];
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

static int parse_line(struct evpaxos_config* c, char* line);
static void address_init(struct address* a, char* addr, int port);
static void address_free(struct address* a);
static void address_copy(struct address* src, struct address* dst);
static struct sockaddr_in address_to_sockaddr(struct address* a);


/**
 * Reads and parses an evpaxos configuration file, populating an evpaxos_config structure.
 *
 * @param path The path to the configuration file.
 * @return A pointer to the populated evpaxos_config structure, or NULL on failure.
 */
struct evpaxos_config* evpaxos_config_read(const char* path)
{
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
	c->pgs = NULL;
	while (fgets(line, sizeof(line), f) != NULL) {
		if (line[0] != '#' && line[0] != '\n' && line[0] != '\r') {
			if (parse_line(c, line) == 0) {
				paxos_log_error("Please, check line %d\n", linenumber);
				paxos_log_error("Please, check line %s\n", line);
				paxos_log_error("Error parsing config file %s\n", path);
				goto failure;
			}
		};
		linenumber++;
	}
	printf("\nFinish readig conf.\n");
	fclose(f);
	return c;

failure:
	free(c);
	if (f != NULL) fclose(f);
	return NULL;
}

/**
 * Returns the number of acceptors (replica nodes) configured in the evpaxos_config structure.
 *
 * @param ref A pointer to the evpaxos_config structure.
 * @return The number of acceptors (replica nodes).
 */
int evpaxos_replica_nodes(struct evpaxos_config* ref) {
	return ref->acceptors_count;
}


/**
 * Frees memory and resources associated with an evpaxos_config structure.
 *
 * @param config A pointer to the evpaxos_config structure to free.
 */
void evpaxos_config_free(struct evpaxos_config* config)
{
	int i;
	for (i = 0; i < config->proposers_count; ++i)
		address_free(&config->proposers[i]);
	for (i = 0; i < config->acceptors_count; ++i)
		address_free(&config->acceptors[i]);
	free(config);
}

/**
 * Retrieves the sockaddr_in representation of a proposer's address from the evpaxos_config structure.
 *
 * @param config A pointer to the evpaxos_config structure.
 * @param i The index of the proposer.
 * @return The sockaddr_in representation of the proposer's address.
 */
struct sockaddr_in evpaxos_proposer_address(struct evpaxos_config* config, int i)
{
	return address_to_sockaddr(&config->proposers[i]);
}

/**
 * Retrieves the listen port of a proposer from the evpaxos_config structure.
 *
 * @param config A pointer to the evpaxos_config structure.
 * @param i The index of the proposer.
 * @return The listen port of the proposer.
 */
int evpaxos_proposer_listen_port(struct evpaxos_config* config, int i)
{
	return config->proposers[i].port;
}

/**
 * Retrieves the number of acceptors (replica nodes) configured in the evpaxos_config structure.
 *
 * @param config A pointer to the evpaxos_config structure.
 * @return The number of acceptors (replica nodes).
 */
int evpaxos_acceptor_count(struct evpaxos_config* config)
{
	return config->acceptors_count;
}


/**
 * Retrieves the sockaddr_in representation of an acceptor's address from the evpaxos_config structure.
 *
 * @param config A pointer to the evpaxos_config structure.
 * @param i The index of the acceptor.
 * @return The sockaddr_in representation of the acceptor's address.
 */
struct sockaddr_in evpaxos_acceptor_address(struct evpaxos_config* config, int i)
{
	return address_to_sockaddr(&config->acceptors[i]);
}

/**
 * Retrieves the listen port of an acceptor from the evpaxos_config structure.
 *
 * @param config A pointer to the evpaxos_config structure.
 * @param i The index of the acceptor.
 * @return The listen port of the acceptor.
 */
int evpaxos_acceptor_listen_port(struct evpaxos_config* config, int i)
{
	return config->acceptors[i].port;
}


/**
 * Trims leading and trailing whitespace characters from a string.
 *
 * @param string The string to trim.
 * @return A pointer to the trimmed portion of the string.
 */
static char* strtrim(char* string)
{
	char *s, *t;
	for (s = string; isspace(*s); s++)
		;
	if (*s == 0)
		return s;
	t = s + strlen(s) - 1;
	while (t > s && isspace(*t))
		t--;
	*++t = '\0';
	return s;
}

/**
 * Parses a string representation of bytes (e.g., "1024", "1kb") and converts it to the corresponding size in bytes.
 * Supports size modifiers "kb", "mb", and "gb".
 *
 * @param str The string representation of bytes.
 * @param bytes A pointer to store the converted size in bytes.
 * @return 1 on successful parsing, 0 on failure.
 */
static int parse_bytes(char* str, size_t* bytes)
{
	char* end;
	errno = 0; /* To distinguish strtoll's return value 0 */
	*bytes = strtoull(str, &end, 10);
	if (errno != 0) return 0;
	while (isspace(*end)) end++;
	if (*end != '\0') {
		if (strcasecmp(end, "kb") == 0) *bytes *= 1024;
		else if (strcasecmp(end, "mb") == 0) *bytes *= 1024 * 1024;
		else if (strcasecmp(end, "gb") == 0) *bytes *= 1024 * 1024 * 1024;
		else return 0;
	}
	return 1;
}

/**
 * Parses a string representation of a boolean value ("yes" or "no") and converts it to an integer (1 or 0).
 *
 * @param str The string representation of a boolean.
 * @param boolean A pointer to store the converted boolean value (1 or 0).
 * @return 1 on successful parsing, 0 on failure.
 */
static int parse_boolean(char* str, int* boolean)
{
	if (str == NULL) return 0;
	if (strcasecmp(str, "yes") == 0) {
		*boolean = 1;
		return 1;
	}
	if (strcasecmp(str, "no") == 0) {
		*boolean = 0;
		return 1;
	}	
	return 0;
}


/**
 * Parses a string representation of an integer and converts it to an integer value.
 *
 * @param str The string representation of an integer.
 * @param integer A pointer to store the converted integer value.
 * @return 1 on successful parsing, 0 on failure.
 */
static int parse_integer(char* str, int* integer)
{
	int n;
	char* end;
	if (str == NULL) return 0;
	n = strtol(str, &end, 10);
	if (end == str) return 0;
	*integer = n;
	return 1;
}

/**
 * parse_string
 *
 * Parses a string and allocates memory for a copy of the string.
 *
 * @param str The input string to parse.
 * @param string A pointer to store the allocated copy of the string.
 * @return 1 on successful parsing, 0 on failure.
 */
static int parse_string(char* str, char** string)
{
	if (str == NULL || str[0] == '\0' || str[0] == '\n')
		return 0;
	*string = strdup(str);
	return 1;
}

/**
 * parse_address
 *
 * Parses a string representing an address in the format "id address port".
 * Initializes an address structure with the parsed information.
 *
 * @param str The input string to parse.
 * @param addr A pointer to the address structure to initialize.
 * @return 1 on successful parsing, 0 on failure.
 */
static int parse_address(char* str, struct address* addr)
{
	int id;
	int port;
	char address[128];
	int rv = sscanf(str, "%d %s %d", &id, address, &port);
	printf("\nparsed %d-%s-%d", id, address, port);
	if (rv == 3) {
		address_init(addr, address, port);
		printf("\nSuccesful parsed");
		return 1;
	}
	printf("\nUnsuccesful parsed");
	return 0;
}

/**
 * parse_verbosity
 *
 * Parses a string representation of verbosity level ("quiet", "error", "info", "debug") and converts it to the corresponding enum value.
 *
 * @param str The string representation of verbosity level.
 * @param verbosity A pointer to store the converted verbosity level.
 * @return 1 on successful parsing, 0 on failure.
 */
static int parse_verbosity(char* str, paxos_log_level* verbosity)
{
	if (strcasecmp(str, "quiet") == 0) *verbosity = PAXOS_LOG_QUIET;
	else if (strcasecmp(str, "error") == 0) *verbosity = PAXOS_LOG_ERROR;
	else if (strcasecmp(str, "info") == 0) *verbosity = PAXOS_LOG_INFO;
	else if (strcasecmp(str, "debug") == 0) *verbosity = PAXOS_LOG_DEBUG;
	else return 0;
	return 1;
}

/**
 * Parses a string representation of a storage backend ("memory" or "lmdb") and converts it to the corresponding enum value.
 *
 * @param str The string representation of storage backend.
 * @param backend A pointer to store the converted storage backend.
 * @return 1 on successful parsing, 0 on failure.
 */
static int parse_backend(char* str, paxos_storage_backend* backend)
{
	if (strcasecmp(str, "memory") == 0) *backend = PAXOS_MEM_STORAGE;
	else if (strcasecmp(str, "lmdb") == 0) *backend = PAXOS_LMDB_STORAGE;
	else return 0;
	return 1;
}

/**
 * Looks up an option by its name in the options array.
 *
 * @param opt The name of the option to look up.
 * @return A pointer to the option structure if found, or NULL if not found.
 */
static struct option* lookup_option(char* opt)
{
	int i = 0;
	while (options[i].name != NULL) {
		if (strcasecmp(options[i].name, opt) == 0)
			return &options[i];
		i++;
	}
	return NULL;
}


/**
 * Parses a configuration line and populates the given evpaxos_config structure based on the content of the line.
 *
 * @param c A pointer to the evpaxos_config structure to populate.
 * @param line The configuration line to parse.
 * @return 1 on successful parsing, 0 on failure.
 */
static int 
parse_line(struct evpaxos_config* c, char* line)
{
	int rv;
	char* tok;
	char* sep = " ";
	struct option* opt;

	line = strtrim(line);
	tok = strsep(&line, sep);
	
	if (strcasecmp(tok, "a") == 0 || strcasecmp(tok, "acceptor") == 0) {
		if (c->acceptors_count >= MAX_N_OF_PROPOSERS) {
			paxos_log_error("Number of acceptors exceded maximum of: %d\n",
				MAX_N_OF_PROPOSERS);
			return 0;
		}
		struct address* addr = &c->acceptors[c->acceptors_count++];
		return parse_address(line, addr);
	}
	
	if (strcasecmp(tok, "p") == 0 || strcasecmp(tok, "proposer") == 0) {
		if (c->proposers_count >= MAX_N_OF_PROPOSERS) {
			paxos_log_error("Number of proposers exceded maximum of: %d\n",
				MAX_N_OF_PROPOSERS);
			return 0;
		}
		struct address* addr = &c->proposers[c->proposers_count++];
		return parse_address(line, addr);
	}
	
	if (strcasecmp(tok, "r") == 0 || strcasecmp(tok, "replica") == 0) {
		if (c->proposers_count >= MAX_N_OF_PROPOSERS ||
			c->acceptors_count >= MAX_N_OF_PROPOSERS ) {
				paxos_log_error("Number of replicas exceded maximum of: %d\n",
					MAX_N_OF_PROPOSERS);
				return 0;
		}
		struct address* pro_addr = &c->proposers[c->proposers_count++];
		struct address* acc_addr = &c->acceptors[c->acceptors_count++];
		int rv = parse_address(line, pro_addr);
		address_copy(pro_addr, acc_addr);
		return rv;
	}

	line = strtrim(line);
	opt = lookup_option(tok);
	if (opt == NULL)
		return 0;

	switch (opt->type) {
		case option_boolean:
			rv = parse_boolean(line, opt->value);
			if (rv == 0) paxos_log_error("Expected 'yes' or 'no'\n");
			break;
		case option_integer:
			rv = parse_integer(line, opt->value);
			if (rv == 0) paxos_log_error("Expected number\n");
			break;
		case option_string:
			rv = parse_string(line, opt->value);
			if (rv == 0) paxos_log_error("Expected string\n");
			break;
		case option_verbosity:
			rv = parse_verbosity(line, opt->value);
			if (rv == 0) paxos_log_error("Expected quiet, error, info, or debug\n");
			break;
		case option_backend:
			rv = parse_backend(line, opt->value);
			if (rv == 0) paxos_log_error("Expected memory or lmdb\n");
			break;
		case option_bytes:
			rv = parse_bytes(line, opt->value);
			if (rv == 0) paxos_log_error("Expected number of bytes.\n");
	}
	
	return rv;
}

/**
 * Initializes an address structure with the given address and port.
 *
 * @param a A pointer to the address structure to initialize.
 * @param addr The string representation of the address.
 * @param port The port number.
 */
static void address_init(struct address* a, char* addr, int port)
{
	a->addr = strdup(addr);
	a->port = port;
}

/**
 * Frees the memory associated with the address string in the address structure.
 *
 * @param a A pointer to the address structure to free.
 */
static void address_free(struct address* a)
{
	free(a->addr);
}

/**
 * Copies the content of the source address structure to the destination address structure.
 *
 * @param src A pointer to the source address structure.
 * @param dst A pointer to the destination address structure.
 */
static void address_copy(struct address* src, struct address* dst)
{
	address_init(dst, src->addr, src->port);
}

/**
 * Converts an address structure to a sockaddr_in structure.
 *
 * @param a A pointer to the address structure to convert.
 * @return A sockaddr_in structure containing the converted address information.
 */
static struct sockaddr_in address_to_sockaddr(struct address* a)
{	
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(a->port);
	addr.sin_addr.s_addr = inet_addr(a->addr);
	return addr;
}

pthread_mutex_t* getPGS(struct evpaxos_config* c)
{
	return c->pgs;
}

//pthread_
void setPGS(struct evpaxos_config* c, pthread_mutex_t* pgs)
{
	c->pgs = pgs;
	//return null;
}