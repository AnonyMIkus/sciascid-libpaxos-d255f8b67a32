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


#include "acceptor.h"
#include "storage.h"
#include <stdlib.h>
#include <string.h>

struct acceptor
{
	int id;
	iid_t trim_iid;
	struct storage store;
};

static void paxos_accepted_to_promise(paxos_accepted* acc, paxos_message* out);
static void paxos_accept_to_accepted(int id, paxos_accept* acc, paxos_message* out);
static void paxos_accepted_to_preempted(int id, paxos_accepted* acc, paxos_message* out);

/**
 * Creates a new instance of the acceptor structure.
 *
 * @param id Identifier for the acceptor.
 * @return Pointer to the newly created acceptor structure, or NULL on failure.
 */
struct acceptor* acceptor_new(int id)
{
	struct acceptor* a; // Prepare Pointer to Acceptor struct.
	a = malloc(sizeof(struct acceptor)); // Allocating Memory for Acceptor.

	// Initialize storage for Acceptor
	storage_init(&a->store, id);

	// Open storage; return NULL on error
	if (storage_open(&a->store) != 0) {
		free(a);
		return NULL;
	}

	// Begin transaction for storage
	if (storage_tx_begin(&a->store) != 0)
		return NULL;
	a->id = id;
	a->trim_iid = storage_get_trim_instance(&a->store);

	// Commit transaction for storage
	if (storage_tx_commit(&a->store) != 0)
		return NULL;
	return a;
}


/**
 * Frees the resources associated with an acceptor structure.
 *
 * @param a Pointer to the acceptor structure to be freed.
 */
void acceptor_free(struct acceptor* a) 
{
	storage_close(&a->store);
	free(a);
}

/**
 * Receives and processes a prepare request from a proposer.
 *
 * @param a Pointer to the acceptor structure.
 * @param req Pointer to the prepare request.
 * @param out Pointer to the paxos_message structure to store the response.
 * @return 1 if the prepare request was successfully processed, 0 otherwise.
 */
int acceptor_receive_prepare(int isrc, struct acceptor* a, paxos_prepare* req, paxos_message* out)
{
	paxos_accepted acc;
	memset(&acc, 0, sizeof(acc));

	if (req->iid <= a->trim_iid)
		return 0;

	memset(&acc, 0, sizeof(paxos_accepted));

	if (storage_tx_begin(&a->store) != 0)
		return 0;

	int found = storage_get_record(&a->store, req->iid, &acc); // Search for exisiting entry. One entry is enough.

	if (!found || acc.ballots[0] <= req->ballot) {
		paxos_log_debug("Preparing iid: %u, ballot: %u", req->iid, req->ballot);
		acc.src = isrc;
		acc.iid = req->iid;
		acc.ballot_0 = req->ballot;
		acc.n_aids = 1;
		acc.aids = calloc(1, sizeof(uint32_t));
		acc.ballots = calloc(1, sizeof(uint32_t));
		acc.value_ballots = calloc(1, sizeof(uint32_t));
		acc.aids[0] = a->id;
		acc.ballots[0] = req->ballot;
		acc.value_ballots[0] = req->ballot;
		acc.values = NULL;

		if (storage_put_record(&a->store, &acc) != 0) {
			storage_tx_abort(&a->store);
			return 0;
		}
	}
	else
	{
		int bfound = 0;
		for (int ii = 0; ii < acc.n_aids; ii++)
		{
			if (acc.aids[ii] == a->id)
			{	
				bfound = 1;
				break;
			}
		}

		if (bfound == 0)
		{
			acc.n_aids++;
			uint32_t* paids = calloc(acc.n_aids, sizeof(uint32_t));
			for (int jj = 0; jj < acc.n_aids - 1; jj++)
			{
				paids[jj] = acc.aids[jj];
			}
			paids[acc.n_aids - 1] = a->id;
			free(acc.aids);
			acc.aids = paids;
			uint32_t* pblt = calloc(acc.n_aids, sizeof(uint32_t));
			for (int jj = 0; jj < acc.n_aids - 1; jj++)
			{
				pblt[jj] = acc.ballots[jj];
			}
			pblt[acc.n_aids - 1] = req->ballot;
			free(acc.ballots);
			acc.ballots = pblt;
			uint32_t* pvb = calloc(acc.n_aids, sizeof(uint32_t));
			for (int jj = 0; jj < acc.n_aids - 1; jj++)
			{
				pvb[jj] = acc.value_ballots[jj];
			}
			pvb[acc.n_aids - 1] = req->ballot;
			free(acc.value_ballots);
			acc.value_ballots = pvb;
			storage_put_record(&a->store, &acc);
		}
	}

	if (storage_tx_commit(&a->store) != 0)
		return 0;


	paxos_accepted_to_promise(&acc, out);
	return 1;
}


/**
 * Receives and processes an accept request from a proposer.
 *
 * @param a Pointer to the acceptor structure.
 * @param req Pointer to the accept request.
 * @param out Pointer to the paxos_message structure to store the response.
 * @return 1 if the accept request was successfully processed, 0 otherwise.
 */
int acceptor_receive_accept(struct acceptor* a, paxos_accept* req, paxos_message* out)
{
	paxos_accepted acc;

	if (req->iid <= a->trim_iid)
		return 0;

	memset(&acc, 0, sizeof(paxos_accepted));

	if (storage_tx_begin(&a->store) != 0)
		return 0;

	int found = storage_get_record(&a->store, req->iid, &acc);

	if (!found || acc.ballots[0] <= req->ballot) {
		paxos_log_debug("Accepting iid: %u, ballot: %u", req->iid, req->ballot);
		paxos_accept_to_accepted(a->id, req, out);

		if (storage_put_record(&a->store, &(out->u.accepted)) != 0) {
			storage_tx_abort(&a->store);
			return 0;
		}
	} else {
		paxos_accepted_to_preempted(a->id, &acc, out);
	}

	if (storage_tx_commit(&a->store) != 0)
		return 0;

	paxos_accepted_destroy(&acc);
	return 1;
}

/**
 * Receives a repeat request for a specific instance ID and retrieves the
 * corresponding accepted value.
 *
 * @param a Pointer to the acceptor structure.
 * @param iid Instance ID for which the accepted value is requested.
 * @param out Pointer to the paxos_accepted structure to store the accepted value.
 * @return 1 if the accepted value was found and successfully retrieved, 0 otherwise.
 */
int acceptor_receive_repeat(struct acceptor* a, iid_t iid, paxos_accepted* out)
{
	memset(out, 0, sizeof(paxos_accepted));
	//paxos_log_debug("out zeroed");

	if (storage_tx_begin(&a->store) != 0)
		return 0;

	//paxos_log_debug("tx begin");
	int found = storage_get_record(&a->store, iid, out);
	//paxos_log_debug("Got record with rc %d ",found);

	if (storage_tx_commit(&a->store) != 0)
		return 0;

	//paxos_log_debug("tx commit");
	if (out->values == NULL)
		return 0;
	//paxos_log_debug("values not null");

	return found && (out->values[0].paxos_value_len > 0);
}

/**
 * Receives a trim request and updates the acceptor's trim instance ID.
 *
 * @param a Pointer to the acceptor structure.
 * @param trim Pointer to the paxos_trim structure containing the trim instance ID.
 * @return 1 if the trim request was successfully processed, 0 otherwise.
 */
int acceptor_receive_trim(struct acceptor* a, paxos_trim* trim)
{
	if (trim->iid <= a->trim_iid)
		return 0;
	a->trim_iid = trim->iid;
	if (storage_tx_begin(&a->store) != 0)
		return 0;
	storage_trim(&a->store, trim->iid);
	if (storage_tx_commit(&a->store) != 0)
		return 0;
	return 1;
}

/**
 * Sets the current state of the acceptor using the provided paxos_acceptor_state structure.
 *
 * @param a Pointer to the acceptor structure.
 * @param state Pointer to the paxos_acceptor_state structure containing the state information.
 */
void acceptor_set_current_state(struct acceptor* a, paxos_acceptor_state* state)
{
	state->aid = a->id;
	state->trim_iid = a->trim_iid;
}

/**
 * Converts a paxos_accepted structure to a paxos_promise structure for response messages.
 *
 * @param acc Pointer to the paxos_accepted structure to be converted.
 * @param out Pointer to the paxos_message structure to store the converted promise message.
 */
static void paxos_accepted_to_promise(paxos_accepted* acc, paxos_message* out)
{
	memcpy(&(out->msg_info[0]), "AAtP", 4);
	out->type = PAXOS_PROMISE;
	out->u.promise = (paxos_promise){
		acc->aids[0],
		acc->iid,
		acc->ballots[0],
		acc->value_ballots[0],
		1,
		calloc(1,sizeof(uint32_t)),
		{0, NULL},
		calloc(1,sizeof(paxos_value)),
		calloc(1,sizeof(uint32_t)),
		calloc(1,sizeof(uint32_t))
	};

	out->u.promise.aids[0] = acc->aids[0];
	if (acc->values != NULL)
	{
		out->u.promise.values[0].paxos_value_len = acc->values[0].paxos_value_len;
		out->u.promise.values[0].paxos_value_val = malloc(acc->values[0].paxos_value_len);
		memcpy(out->u.promise.values[0].paxos_value_val, acc->values[0].paxos_value_val, acc->values[0].paxos_value_len);
	};
	out->u.promise.ballots[0] = acc->ballots[0];
	out->u.promise.value_ballots[0] = acc->value_ballots[0];
}

/**
 * Converts a paxos_accept structure to a paxos_accepted structure for accepted messages.
 *
 * @param id The ID of the acceptor.
 * @param acc Pointer to the paxos_accept structure to be converted.
 * @param out Pointer to the paxos_message structure to store the converted accepted message.
 */
static void paxos_accept_to_accepted(int id, paxos_accept* acc, paxos_message* out)
{
	memcpy(&(out->msg_info[0]), "AAtA", 4);
	out->type = PAXOS_ACCEPTED;
	out->u.accepted = (paxos_accepted) {
		id,
		acc->iid,
		acc->ballot,
		acc->ballot,
		1,
		calloc(1,sizeof(uint32_t)),
		// {value_size, value},
		{0,NULL},
		calloc(1,sizeof(paxos_value)),
		calloc(1,sizeof(uint32_t)),
		calloc(1,sizeof(uint32_t))
	};
	out->u.accepted.aids[0] = id;
	out->u.accepted.values[0].paxos_value_len = acc->value.paxos_value_len;
	out->u.accepted.values[0].paxos_value_val = malloc(acc->value.paxos_value_len);
	memcpy(out->u.accepted.values[0].paxos_value_val, acc->value.paxos_value_val, acc->value.paxos_value_len);
	out->u.accepted.ballots[0]=acc->ballot;
	out->u.accepted.value_ballots[0] = acc->ballot;
}

/**
 * Converts a paxos_accepted structure to a paxos_preempted structure for preempted messages.
 *
 * @param id The ID of the acceptor.
 * @param acc Pointer to the paxos_accepted structure to be converted.
 * @param out Pointer to the paxos_message structure to store the converted preempted message.
 */
static void paxos_accepted_to_preempted(int id, paxos_accepted* acc, paxos_message* out)
{
	out->type = PAXOS_PREEMPTED;
	out->u.preempted = (paxos_preempted) { 
		id, acc->iid, acc->ballots[0]
	};
}

int get_srcid_promise_and_adjust(paxos_promise* pr, struct acceptor* a)
{
	int ret = -1;
	paxos_accepted acc;
	memset(&acc, 0, sizeof(paxos_accepted));

	if (storage_tx_begin(&a->store) != 0)
		return -1;

	int found = storage_get_record(&a->store, pr->iid, &acc);

	if (found)
	{
		ret = acc.src;

		int bfound = 0;
		for (int ii = 0; ii < acc.n_aids; ii++)
		{
			if (acc.aids[ii] == pr->iid)
			{
				bfound = 1;
				break;
			}
		}

		if (bfound == 0)
		{
			acc.n_aids++;
			uint32_t* paids = calloc(acc.n_aids, sizeof(uint32_t));
			for (int jj = 0; jj < acc.n_aids - 1; jj++)
			{
				paids[jj] = acc.aids[jj];
			}
			paids[acc.n_aids - 1] = pr->iid;
			free(acc.aids);
			acc.aids = paids;
			uint32_t* pblt = calloc(acc.n_aids, sizeof(uint32_t));
			for (int jj = 0; jj < acc.n_aids - 1; jj++)
			{
				pblt[jj] = acc.ballots[jj];
			}
			pblt[acc.n_aids - 1] = pr->ballots[0];
			free(acc.ballots);
			acc.ballots = pblt;
			uint32_t* pvb = calloc(acc.n_aids, sizeof(uint32_t));
			for (int jj = 0; jj < acc.n_aids - 1; jj++)
			{
				pvb[jj] = acc.value_ballots[jj];
			}
			pvb[acc.n_aids - 1] = pr->value_ballots[0];
			free(acc.value_ballots);
			acc.value_ballots = pvb;
			storage_put_record(&a->store, &acc);
		}
	}
	if (storage_tx_commit(&a->store) != 0)
		return -1;

	return ret;
}

int get_srcid_accepted(paxos_accepted* ac, struct acceptor* a)
{
	int ret = -1;
	paxos_accepted acc;
	memset(&acc, 0, sizeof(paxos_accepted));

	if (storage_tx_begin(&a->store) != 0)
		return -1;

	int found = storage_get_record(&a->store, ac->iid, &acc);

	if (found)
		ret = acc.src;

	if (storage_tx_commit(&a->store) != 0)
		return -1;

	return ret;
}

int get_srcid_preempted(paxos_preempted* ac, struct acceptor* a)
{
	int ret = -1;
	paxos_accepted acc;
	memset(&acc, 0, sizeof(paxos_accepted));

	if (storage_tx_begin(&a->store) != 0)
		return -1;

	int found = storage_get_record(&a->store, ac->iid, &acc);

	if (found)
		ret = acc.src;

	if (storage_tx_commit(&a->store) != 0)
		return -1;

	return ret;
}