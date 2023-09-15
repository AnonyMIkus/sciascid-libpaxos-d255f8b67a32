/*
 * Copyright (c) 2014, University of Lugano
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


#include "storage_utils.h"
#include <stdlib.h>
#include <string.h>

char*
paxos_accepted_to_buffer(paxos_accepted* acc)
{
	size_t len = acc->n_aids * sizeof(uint32_t);
	for (int i = 0; i < acc->n_aids; i++)
	{
		len += acc->values[i].paxos_value_len+sizeof(uint32_t);
	}

	char* buffer = malloc(sizeof(paxos_accepted) + len);
	if (buffer == NULL)
		return NULL;
	memcpy(buffer, acc, sizeof(paxos_accepted));
	memcpy(buffer+sizeof(paxos_accepted), &(acc->aids[0]), acc->n_aids * sizeof(uint32_t));
	char* p = buffer + sizeof(paxos_accepted) + acc->n_aids * sizeof(uint32_t);
	for(int i=0;i< acc->n_aids;i++)
	{ 
		memcpy(p, &(acc->values[i].paxos_value_len), sizeof(uint32_t));
		p += sizeof(uint32_t);
		memcpy(p, &(acc->values[i].paxos_value_val), acc->values[i].paxos_value_len);
		p += acc->values[i].paxos_value_len;
	}
	return buffer;
}

void
paxos_accepted_from_buffer(char* buffer, paxos_accepted* out)
{
	memcpy(out, buffer, sizeof(paxos_accepted));
	if (out->n_aids > 0)
	{
		out->aids = malloc(out->n_aids * sizeof(uint32_t));
		memcpy(out->aids, buffer + sizeof(paxos_accepted), out->n_aids * sizeof(uint32_t));
		out->values = malloc(out->n_aids * sizeof(paxos_value));
		char* p = buffer + sizeof(paxos_accepted) + out->n_aids * sizeof(uint32_t);
		for (int i = 0; i < out->n_aids; i++)
		{
			memcpy(&(out->values[i].paxos_value_len), p, sizeof(uint32_t));
			p += sizeof(uint32_t);
			out->values[i].paxos_value_val = malloc(out->values[i].paxos_value_len);
			memcpy(out->values[i].paxos_value_val, p, out->values[i].paxos_value_len);
			p += out->values[i].paxos_value_len;

		}

	}
}
