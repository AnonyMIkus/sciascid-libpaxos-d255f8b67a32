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


#ifndef _CONFIG_READER_H_
#define _CONFIG_READER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../paxos/include/paxos.h"

	struct address
	{
		char* addr;
		int port;
		int groupid;
		int parentid;
	};

	struct evpaxos_config
	{
		int proposers_count;
		int acceptors_count;
		struct address proposers[MAX_N_OF_PROPOSERS];
		struct address acceptors[MAX_N_OF_PROPOSERS];
		pthread_mutex_t* pgs;
	};

struct evpaxos_config* evpaxos_config_read(const char* path);
void evpaxos_config_free(struct evpaxos_config* config);
struct sockaddr_in evpaxos_proposer_address(struct evpaxos_config* c, int i);
int evpaxos_proposer_listen_port(struct evpaxos_config* c, int i);
int evpaxos_acceptor_count(struct evpaxos_config* config);
struct sockaddr_in evpaxos_acceptor_address(struct evpaxos_config* c, int i);
int evpaxos_acceptor_listen_port(struct evpaxos_config* c, int i);

pthread_mutex_t* getPGS(struct evpaxos_config* c);
void setPGS(struct evpaxos_config* c,pthread_mutex_t* pgs);

#ifdef __cplusplus
}
#endif

#endif
