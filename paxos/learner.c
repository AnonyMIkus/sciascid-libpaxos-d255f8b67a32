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


#include "learner.h"
#include "khash.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct instance
{
	iid_t iid;
	ballot_t last_update_ballot;
	paxos_accepted** acks;
	paxos_accepted* final_value;
};
/** Define a hash map for instances with integer keys */
KHASH_MAP_INIT_INT(instance, struct instance*)

struct learner
{
	int acceptors;
	int late_start;
	iid_t current_iid;
	iid_t highest_iid_closed;
	khash_t(instance)* instances;
};

static struct instance* learner_get_instance(struct learner* l, iid_t iid);
static struct instance* learner_get_current_instance(struct learner* l);
static struct instance* learner_get_instance_or_create(struct learner* l, iid_t iid);
static void learner_delete_instance(struct learner* l, struct instance* inst);
static struct instance* instance_new(int acceptors);
static void instance_free(struct instance* i, int acceptors);
static void instance_update(struct instance* i, paxos_accepted* ack, int acceptors);
static int instance_has_quorum(struct instance* i, int acceptors);
static void instance_add_accept(struct instance* i, paxos_accepted* ack);
static paxos_accepted* paxos_accepted_dup(paxos_accepted* ack);
static void paxos_value_copy(paxos_value* dst, paxos_value* src);
static void paxos_accepted_deep_copy(paxos_accepted* ack, paxos_accepted* copy);

/**
 * Creates a new learner instance.
 *
 * @param acceptors Number of acceptors in the Paxos system.
 * @return Pointer to the newly created learner instance.
 */
struct learner* learner_new(int acceptors)
{
	struct learner* l;
	l = malloc(sizeof(struct learner));
	l->acceptors = acceptors;
	l->current_iid = 1;
	l->highest_iid_closed = 1;
	l->late_start = !paxos_config.learner_catch_up;
	l->instances = kh_init(instance);
	return l;
}

/**
 * Frees the memory occupied by a learner instance and its associated resources.
 *
 * @param l Pointer to the learner instance to be freed.
 */
void learner_free(struct learner* l)
{
	struct instance* inst;
	kh_foreach_value(l->instances, inst, instance_free(inst, l->acceptors));
	kh_destroy(instance, l->instances);
	free(l);
}


/**
 * Sets the current instance ID for the learner.
 *
 * @param l Pointer to the learner instance.
 * @param iid Instance ID to be set.
 */
void learner_set_instance_id(struct learner* l, iid_t iid)
{
	l->current_iid = iid + 1;
	l->highest_iid_closed = iid;
}

/**
 * Handles the reception of an accepted message by the learner.
 *
 * @param l Pointer to the learner instance.
 * @param ack Pointer to the received paxos_accepted message.
 */
void learner_receive_accepted(struct learner* l, paxos_accepted* ack)
{	
	paxos_log_debug("entering paxos learner %lx", l);

	if (l->late_start) {
		l->late_start = 0; // Turn off late start flag
		l->current_iid = ack->iid; // Set current instance ID
	}
	paxos_log_debug("learner %lx stage1", l);

	if (ack->iid < l->current_iid) {
		paxos_log_debug("Dropped paxos_accepted for iid %u. Already delivered.",
			ack->iid);
		return;
	}
	paxos_log_debug("learner %lx stage2", l);

	struct instance* inst;
	inst = learner_get_instance_or_create(l, ack->iid);
	
	// Update instance with accepted message
	instance_update(inst, ack, l->acceptors);
	paxos_log_debug("learner %lx stage3", l);

	// If instance has a quorum and is not yet closed, update highest closed instance ID
	if (instance_has_quorum(inst, l->acceptors)
		&& (inst->iid > l->highest_iid_closed))
		l->highest_iid_closed = inst->iid;
	paxos_log_debug("learner %lx stage4", l);

}
/**
 * Attempts to deliver the next accepted value from the learner's instance queue.
 *
 * @param l Pointer to the learner instance.
 * @param out Pointer to store the delivered paxos_accepted value.
 * @return 1 if a value was successfully delivered, 0 otherwise.
 */
int learner_deliver_next(struct learner* l, paxos_accepted* out)
{
	paxos_log_debug("learner %lx deliever next stage1", l);

	struct instance* inst = learner_get_current_instance(l);
	// If no current instance or quorum is missing, cannot deliver
	if (inst == NULL || !instance_has_quorum(inst, l->acceptors))
		return 0;
	// Copy the final value and mark it as delivered
	paxos_log_debug("learner %lx deliever next stage2", l);

//	memcpy(out, inst->final_value, sizeof(paxos_accepted));
//	paxos_value_copy(&out->value, &inst->final_value->value);
	paxos_accepted_deep_copy(inst->final_value, out);
	learner_delete_instance(l, inst);
	l->current_iid++;
	paxos_log_debug("learner %lx deliever next stage3", l);

	return 1;
}

/**
 * Checks if there are any "holes" in the learner's instance sequence.
 *
 * @param l Pointer to the learner instance.
 * @param from Pointer to store the starting instance ID of the hole.
 * @param to Pointer to store the ending instance ID of the hole.
 * @return 1 if holes are found, 0 otherwise.
 */
int learner_has_holes(struct learner* l, iid_t* from, iid_t* to)
{
	if (l->highest_iid_closed > l->current_iid) {
		*from = l->current_iid;
		*to = l->highest_iid_closed;
		return 1;// Holes are found
	}
	return 0; // No holes
}

/**
 * Retrieves an instance with a specific instance ID from the learner's instance map.
 *
 * @param l Pointer to the learner instance.
 * @param iid Instance ID to retrieve.
 * @return Pointer to the retrieved instance, or NULL if not found.
 */
static struct instance* learner_get_instance(struct learner* l, iid_t iid)
{
	khiter_t k;
	k = kh_get_instance(l->instances, iid);
	if (k == kh_end(l->instances))
		return NULL;
	return kh_value(l->instances, k);
}

/**
 * Retrieves the current instance of the learner.
 *
 * @param l Pointer to the learner instance.
 * @return Pointer to the current instance, or NULL if not found.
 */
static struct instance* learner_get_current_instance(struct learner* l)
{
	return learner_get_instance(l, l->current_iid);
}

/**
 * Retrieves an instance with a specific instance ID from the learner's instance map.
 * If the instance does not exist, creates a new instance and adds it to the map.
 *
 * @param l Pointer to the learner instance.
 * @param iid Instance ID to retrieve or create.
 * @return Pointer to the retrieved or newly created instance.
 */
static struct instance* learner_get_instance_or_create(struct learner* l, iid_t iid)
{
	struct instance* inst = learner_get_instance(l, iid);
	if (inst == NULL) {
		int rv;
		khiter_t k = kh_put_instance(l->instances, iid, &rv); // Store the instance in the map
		assert(rv != -1);
		inst = instance_new(l->acceptors);
		kh_value(l->instances, k) = inst;
	}
	return inst;
}

/**
 * Deletes an instance from the learner's instance map and frees its resources.
 *
 * @param l Pointer to the learner instance.
 * @param inst Pointer to the instance to be deleted.
 */
static void learner_delete_instance(struct learner* l, struct instance* inst)
{
	khiter_t k;
	k = kh_get_instance(l->instances, inst->iid);
	kh_del_instance(l->instances, k);	// Remove instance from the map
	instance_free(inst, l->acceptors);	// Free resources associated with the instance
}

/**
 * Creates a new instance with allocated memory for the acknowledgments.
 *
 * @param acceptors Number of acceptors for which acknowledgments will be stored.
 * @return Pointer to the newly created instance.
 */
static struct instance* instance_new(int acceptors)
{
	int i;
	struct instance* inst;
	inst = malloc(sizeof(struct instance));
	memset(inst, 0, sizeof(struct instance));
	inst->acks = malloc(sizeof(paxos_accepted*) * acceptors); // Allocate memory for acks
	for (i = 0; i < acceptors; ++i)
		inst->acks[i] = NULL;
	return inst;
}

/**
 * Frees the resources associated with an instance, including the acks array.
 *
 * @param inst Pointer to the instance to be freed.
 * @param acceptors Number of acceptors for which acknowledgments are stored.
 */
static void instance_free(struct instance* inst, int acceptors)
{
	int i;
	for (i = 0; i < acceptors; i++)
		if (inst->acks[i] != NULL) paxos_accepted_free(inst->acks[i]); // Free individual acks
	free(inst->acks);	// Free the acks array
	free(inst);			// Free the instance itself
}

/**
 * Updates an instance with a received accepted value.
 *
 * @param inst Pointer to the instance to be updated.
 * @param accepted Pointer to the received paxos_accepted value.
 * @param acceptors Number of acceptors for which acknowledgments are stored.
 */
static void instance_update(struct instance* inst, paxos_accepted* accepted, int acceptors)
{	
	// Initialize the instance if it's the first message received for it
	if (inst->iid == 0) {
		paxos_log_debug("Received first message for iid: %u", accepted->iid);
		inst->iid = accepted->iid;
		inst->last_update_ballot = accepted->ballot;
	}
	
	// If the instance is already closed, drop the message
	if (instance_has_quorum(inst, acceptors)) {
		paxos_log_debug("Dropped paxos_accepted iid %u. Already closed.",
			accepted->iid);
		return;
	}
	
	// Check if the received ballot is newer than the previous ballot
	paxos_accepted* prev_accepted = inst->acks[accepted->aid];
	if (prev_accepted != NULL && prev_accepted->ballot >= accepted->ballot) {
		paxos_log_debug("Dropped paxos_accepted for iid %u."
			"Previous ballot is newer or equal.", accepted->iid);
		return;
	}
	
	// Add the received accepted value to the instance's acks array
	instance_add_accept(inst, accepted);
}

/* 
	Checks if a given instance is closed, that is if a quorum of acceptor 
	accepted the same value ballot pair. 
	Returns 1 if the instance is closed, 0 otherwise.
*/

/**
 * Checks if the instance has achieved a quorum for acceptance.
 *
 * @param inst Pointer to the instance to be checked.
 * @param acceptors Number of acceptors in the system.
 * @return 1 if a quorum has been reached, 0 otherwise.
 */
static int instance_has_quorum(struct instance* inst, int acceptors)
{
	paxos_accepted* curr_ack;
	int i, a_valid_index = -1, count = 0;

	// If a final value has been set, the instance is considered closed
	if (inst->final_value != NULL)
		return 1;

	// Iterate through acceptor acknowledgments
	for (i = 0; i < acceptors; i++) {
		curr_ack = inst->acks[i];
	
		// Skip over missing acceptor acks
		if (curr_ack == NULL) continue;
		
		// Count the ones "agreeing" with the last added
		if (curr_ack->ballot == inst->last_update_ballot) {
			count++;
			a_valid_index = i;
		}
	}

	// Check if a quorum has been reached and set the final value if so
	if (count >= paxos_quorum(acceptors)) {
		paxos_log_debug("Reached quorum, iid: %u is closed!", inst->iid);
		inst->final_value = inst->acks[a_valid_index];
		return 1;
	}
	return 0;
}

/*
	Adds the given paxos_accepted to the given instance, 
	replacing the previous paxos_accepted, if any.
*/

/**
 * Adds an accepted value to the instance's acknowledgments array.
 *
 * @param inst Pointer to the instance to be updated.
 * @param accepted Pointer to the accepted value to be added.
 */
static void instance_add_accept(struct instance* inst, paxos_accepted* accepted)
{
	paxos_log_debug("learner %lx add accept stage1",inst );

	int acceptor_id = accepted->aid;
	if (inst->acks[acceptor_id] != NULL)
		paxos_accepted_free(inst->acks[acceptor_id]);
	inst->acks[acceptor_id] = paxos_accepted_dup(accepted);
	inst->last_update_ballot = accepted->ballot;
	paxos_log_debug("learner %lx add accept stage2", inst);

}

/*
	Returns a copy of it's argument.
*/

/**
 * Creates a copy of a paxos_accepted structure.
 *
 * @param ack Pointer to the paxos_accepted structure to be duplicated.
 * @return Pointer to the newly duplicated paxos_accepted structure.
 */
static paxos_accepted* paxos_accepted_dup(paxos_accepted* ack)
{
	paxos_log_debug("learner %lx add copy stage1", ack);

	paxos_accepted* copy;
	copy = malloc(sizeof(paxos_accepted));
	memcpy(copy, ack, sizeof(paxos_accepted));
	paxos_value_copy(&copy->value, &ack->value);
	paxos_log_debug("learner %lx add copy stage2", ack);
	if (copy->n_aids > 0)
	{
		if (ack->aids != NULL) copy->aids = calloc(copy->n_aids, sizeof(uint32_t)); else ack->aids = NULL;
		if (ack->values != NULL) copy->values = calloc(copy->n_aids, sizeof(paxos_value)); else ack->values = NULL;

		for (int ii = 0; ii < copy->n_aids; ii++)
		{
			if (ack->aids != NULL)  copy->aids[ii] = ack->aids[ii];
			if (ack->values != NULL) paxos_value_copy(&copy->values[ii], &ack->values[ii]);
		}

	}

	return copy;
}

static void paxos_accepted_deep_copy(paxos_accepted* ack, paxos_accepted* copy)
{
	paxos_log_debug("learner %lx add copy stage1", ack);

	memcpy(copy, ack, sizeof(paxos_accepted));
	paxos_value_copy(&copy->value, &ack->value);
	paxos_log_debug("learner %lx add copy stage2", ack);
	if (copy->n_aids > 0)
	{
		if (ack->aids != NULL) copy->aids = calloc(copy->n_aids, sizeof(uint32_t)); else ack->aids = NULL;
		if (ack->values != NULL) copy->values = calloc(copy->n_aids, sizeof(paxos_value)); else ack->values = NULL;

		for (int ii = 0; ii < copy->n_aids; ii++)
		{
			if (ack->aids != NULL)  copy->aids[ii] = ack->aids[ii];
			if (ack->values != NULL) paxos_value_copy(&copy->values[ii], &ack->values[ii]);
		}

	}
}

/**
 * Copies the value from one paxos_value structure to another.
 *
 * @param dst Pointer to the destination paxos_value structure.
 * @param src Pointer to the source paxos_value structure.
 */
static void paxos_value_copy(paxos_value* dst, paxos_value* src)
{
	int len = src->paxos_value_len;
	dst->paxos_value_len = len;
	if (src->paxos_value_val != NULL) {
		dst->paxos_value_val = malloc(len);
		memcpy(dst->paxos_value_val, src->paxos_value_val, len);	
	}
}
