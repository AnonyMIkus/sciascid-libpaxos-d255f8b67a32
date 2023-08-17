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
#include "quorum.h"
#include <stdlib.h>
#include <string.h>

 /**
  * Initializes the quorum structure with the given number of acceptors.
  *
  * @param q Pointer to the quorum structure to initialize.
  * @param acceptors Total number of acceptors in the system.
  */
void quorum_init(struct quorum* q, int acceptors)
{
	q->acceptors = acceptors;
	q->quorum = paxos_quorum(acceptors);
	q->acceptor_ids = malloc(sizeof(int) * q->acceptors);
	quorum_clear(q);
}

/**
 * Clears the quorum state by resetting the acceptor presence tracking.
 *
 * @param q Pointer to the quorum structure to clear.
 */
void quorum_clear(struct quorum* q)
{
	q->count = 0;
	memset(q->acceptor_ids, 0, sizeof(int) * q->acceptors); // Reset the acceptor presence tracking.
}

/**
 * Frees memory allocated for the quorum structure.
 *
 * @param q Pointer to the quorum structure to destroy.
 */
void quorum_destroy(struct quorum* q)
{
	free(q->acceptor_ids);
}

/**
 * Adds an acceptor to the quorum if not already added.
 *
 * @param q Pointer to the quorum structure.
 * @param id ID of the acceptor to add.
 * @return 1 if the acceptor was successfully added, 0 if it was already present.
 */
int quorum_add(struct quorum* q, int id)
{
	if (q->acceptor_ids[id] == 0) {
		q->count++;
		q->acceptor_ids[id] = 1;
		return 1;
	}
	return 0;
}

/**
 * Checks if the quorum size has been reached.
 *
 * @param q Pointer to the quorum structure.
 * @return 1 if the quorum size has been reached, 0 otherwise.
 */
int quorum_reached(struct quorum* q)
{
	return (q->count >= q->quorum); // Check if the count of acceptors in the quorum is sufficient.
}
