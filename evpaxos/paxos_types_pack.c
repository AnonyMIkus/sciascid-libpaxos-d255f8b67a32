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


#include "paxos_types_pack.h"

 /**
  * A macro used to access elements within a MessagePack array or map at the specified index.
  *
  * @param obj A pointer to the msgpack_object containing the array or map.
  * @param i The index of the element to access.
  * @return Returns the accessed element at the specified index.
  */
#define MSGPACK_OBJECT_AT(obj, i) (obj->via.array.ptr[i].via)


/**
* Packs a string into a MessagePack buffer using the given packer.
*
* @param p Pointer to the MessagePack packer.
* @param buffer Pointer to the string to be packed.
* @param len Length of the string to be packed.
*/
static void msgpack_pack_string(msgpack_packer* p, char* buffer, int len)
{
	// Check the MessagePack version to determine the appropriate packing function
	#if MSGPACK_VERSION_MAJOR > 0
	msgpack_pack_bin(p, len);
	msgpack_pack_bin_body(p, buffer, len);
	#else
	msgpack_pack_raw(p, len);
	msgpack_pack_raw_body(p, buffer, len);
	#endif
}

/**
 * Unpacks a uint32_t value from a MessagePack object at the specified index.
 *
 * @param o Pointer to the msgpack_object containing the value.
 * @param v Pointer to the variable where the unpacked uint32_t value will be stored.
 * @param i Pointer to the index in the object's array or map to unpack from.
 */
static void msgpack_unpack_uint32_at(msgpack_object* o, uint32_t* v, int* i)
{
	*v = (uint32_t)MSGPACK_OBJECT_AT(o,*i).u64;
	(*i)++;
}

/**
 * Unpacks a string from a MessagePack object at the specified index.
 *
 * @param o Pointer to the msgpack_object containing the value.
 * @param buffer Pointer to the variable where the unpacked string will be stored.
 * @param len Pointer to the variable where the length of the unpacked string will be stored.
 * @param i Pointer to the index in the object's array or map to unpack from.
 */
static void msgpack_unpack_string_at(msgpack_object* o, char** buffer, int* len, int* i)
{
	*buffer = NULL;
	#if MSGPACK_VERSION_MAJOR > 0
	*len = MSGPACK_OBJECT_AT(o,*i).bin.size;
	const char* obj = MSGPACK_OBJECT_AT(o,*i).bin.ptr;
	#else
	*len = MSGPACK_OBJECT_AT(o,*i).raw.size;
	const char* obj = MSGPACK_OBJECT_AT(o,*i).raw.ptr;
	#endif
	if (*len > 0) {
		*buffer = malloc(*len);
		memcpy(*buffer, obj, *len);
	}
	(*i)++;
}

/**
 * Packs a paxos_value structure into a MessagePack buffer using the given packer.
 *
 * @param p Pointer to the MessagePack packer.
 * @param v Pointer to the paxos_value structure to be packed.
 */
static void msgpack_pack_paxos_value(msgpack_packer* p, paxos_value* v)
{
	msgpack_pack_string(p, v->paxos_value_val, v->paxos_value_len);
}

/**
 * Unpacks a paxos_value structure from a MessagePack object at the specified index.
 *
 * @param o Pointer to the msgpack_object containing the paxos_value structure.
 * @param v Pointer to the paxos_value structure where the unpacked data will be stored.
 * @param i Pointer to the index in the object's array or map to unpack from.
 */
static void msgpack_unpack_paxos_value_at(msgpack_object* o, paxos_value* v, int* i)
{
	msgpack_unpack_string_at(o, &v->paxos_value_val, &v->paxos_value_len, i);
}

/**
 * Packs a paxos_prepare structure into a MessagePack buffer using the given packer.
 *
 * @param p Pointer to the MessagePack packer.
 * @param v Pointer to the paxos_prepare structure to be packed.
 */
void msgpack_pack_paxos_prepare(msgpack_packer* p, paxos_prepare* v)
{
	msgpack_pack_array(p, 3); // Start packing an array with 3 elements
	msgpack_pack_int32(p, PAXOS_PREPARE); // Pack the PAXOS_PREPARE constant
	msgpack_pack_uint32(p, v->iid); // Pack the instance ID
	msgpack_pack_uint32(p, v->ballot); // Pack the ballot
}

/**
 * Unpacks a paxos_prepare structure from a MessagePack object.
 *
 * @param o Pointer to the msgpack_object containing the paxos_prepare structure.
 * @param v Pointer to the paxos_prepare structure where the unpacked data will be stored.
 */
void msgpack_unpack_paxos_prepare(msgpack_object* o, paxos_prepare* v)
{
	int i = 1;
	// Unpack the instance ID and ballot from the MessagePack object
	msgpack_unpack_uint32_at(o, &v->iid, &i);
	msgpack_unpack_uint32_at(o, &v->ballot, &i);
}

/**
 * Packs a paxos_promise structure into a MessagePack buffer using the given packer.
 *
 * @param p Pointer to the MessagePack packer.
 * @param v Pointer to the paxos_promise structure to be packed.
 */
void msgpack_pack_paxos_promise(msgpack_packer* p, paxos_promise* v)
{
	msgpack_pack_array(p, 7+v->n_aids+v->n_aids+v->n_aids+v->n_aids); // Start packing an array with 7 elements
	msgpack_pack_int32(p, PAXOS_PROMISE);
	msgpack_pack_uint32(p, v->aid_0);
	msgpack_pack_uint32(p, v->iid);
	msgpack_pack_uint32(p, v->ballot_0);
	msgpack_pack_uint32(p, v->value_ballot_0);
	msgpack_pack_uint32(p, v->n_aids);
	msgpack_pack_paxos_value(p, &v->value_0);
//	msgpack_pack_array(p, v->n_aids); 
	for (int i = 0; i < v->n_aids; i++) 
		msgpack_pack_uint32(p, v->aids[i]); 
	for (int i = 0; i < v->n_aids; i++)
		msgpack_pack_paxos_value(p, &(v->values[i]));
	for (int i = 0; i < v->n_aids; i++)
		msgpack_pack_uint32(p, v->ballots[i]);
	for (int i = 0; i < v->n_aids; i++)
		msgpack_pack_uint32(p, v->value_ballots[i]);
}

/**
 * Unpacks a paxos_promise structure from a MessagePack object.
 *
 * @param o Pointer to the msgpack_object containing the paxos_promise structure.
 * @param v Pointer to the paxos_promise structure where the unpacked data will be stored.
 */
void msgpack_unpack_paxos_promise(msgpack_object* o, paxos_promise* v)
{
	int i = 1;
	msgpack_unpack_uint32_at(o, &v->aid_0, &i);
	msgpack_unpack_uint32_at(o, &v->iid, &i);
	msgpack_unpack_uint32_at(o, &v->ballot_0, &i);
	msgpack_unpack_uint32_at(o, &v->value_ballot_0, &i);
	msgpack_unpack_uint32_at(o, &v->n_aids, &i);
	msgpack_unpack_paxos_value_at(o, &v->value_0, &i); // Unpack the paxos_value structure
	v->aids = calloc(v->n_aids, sizeof(uint32_t));
	for (int ii = 0; ii < v->n_aids; ii++)
		msgpack_unpack_uint32_at(o, &v->aids[ii], &i);
	v->values = calloc(v->n_aids, sizeof(paxos_value));
	for (int ii = 0; ii < v->n_aids; ii++)
		msgpack_unpack_paxos_value_at(o, &v->values[ii], &i);
	v->ballots = calloc(v->n_aids, sizeof(uint32_t));
	v->value_ballots = calloc(v->n_aids, sizeof(uint32_t));
	for (int ii = 0; ii < v->n_aids; ii++)
		msgpack_unpack_uint32_at(o, &v->ballots[ii], &i);
	for (int ii = 0; ii < v->n_aids; ii++)
		msgpack_unpack_uint32_at(o, &v->value_ballots[ii], &i);
}

/**
 * Packs a paxos_accept structure into a MessagePack buffer using the given packer.
 *
 * @param p Pointer to the MessagePack packer.
 * @param v Pointer to the paxos_accept structure to be packed.
 */
void msgpack_pack_paxos_accept(msgpack_packer* p, paxos_accept* v)
{
	msgpack_pack_array(p, 4); // Start packing an array with 4 elements
	msgpack_pack_int32(p, PAXOS_ACCEPT);
	msgpack_pack_uint32(p, v->iid);
	msgpack_pack_uint32(p, v->ballot);
	msgpack_pack_paxos_value(p, &v->value);
}

/**
 * Unpacks a paxos_accept structure from a MessagePack object.
 *
 * @param o Pointer to the msgpack_object containing the paxos_accept structure.
 * @param v Pointer to the paxos_accept structure where the unpacked data will be stored.
 */
void msgpack_unpack_paxos_accept(msgpack_object* o, paxos_accept* v)
{
	int i = 1;
	msgpack_unpack_uint32_at(o, &v->iid, &i);
	msgpack_unpack_uint32_at(o, &v->ballot, &i);
	msgpack_unpack_paxos_value_at(o, &v->value, &i);
}

/**
 * Packs a paxos_accepted structure into a MessagePack buffer using the given packer.
 *
 * @param p Pointer to the MessagePack packer.
 * @param v Pointer to the paxos_accepted structure to be packed.
 */
void msgpack_pack_paxos_accepted(msgpack_packer* p, paxos_accepted* v)
{
	msgpack_pack_array(p, 7);
	msgpack_pack_int32(p, PAXOS_ACCEPTED);
	msgpack_pack_uint32(p, v->aid);
	msgpack_pack_uint32(p, v->iid);
	msgpack_pack_uint32(p, v->ballot);
	msgpack_pack_uint32(p, v->value_ballot);
	msgpack_pack_uint32(p, v->n_aids);
	msgpack_pack_paxos_value(p, &v->value);
}

/**
 * Unpacks a paxos_accepted structure from a MessagePack object.
 *
 * @param o Pointer to the msgpack_object containing the paxos_accepted structure.
 * @param v Pointer to the paxos_accepted structure where the unpacked data will be stored.
 */
void msgpack_unpack_paxos_accepted(msgpack_object* o, paxos_accepted* v)
{
	int i = 1;
	msgpack_unpack_uint32_at(o, &v->aid, &i);
	msgpack_unpack_uint32_at(o, &v->iid, &i);
	msgpack_unpack_uint32_at(o, &v->ballot, &i);
	msgpack_unpack_uint32_at(o, &v->value_ballot, &i);
	msgpack_unpack_uint32_at(o, &v->n_aids, &i);
	msgpack_unpack_paxos_value_at(o, &v->value, &i);
}

/**
 * Packs a paxos_preempted structure into a MessagePack buffer using the given packer.
 *
 * @param p Pointer to the MessagePack packer.
 * @param v Pointer to the paxos_preempted structure to be packed.
 */
void msgpack_pack_paxos_preempted(msgpack_packer* p, paxos_preempted* v)
{
	msgpack_pack_array(p, 4);
	msgpack_pack_int32(p, PAXOS_PREEMPTED);
	msgpack_pack_uint32(p, v->aid);
	msgpack_pack_uint32(p, v->iid);
	msgpack_pack_uint32(p, v->ballot);
}

/**
 * Unpacks a paxos_preempted structure from a MessagePack object.
 *
 * @param o Pointer to the msgpack_object containing the paxos_preempted structure.
 * @param v Pointer to the paxos_preempted structure where the unpacked data will be stored.
 */
void msgpack_unpack_paxos_preempted(msgpack_object* o, paxos_preempted* v)
{
	int i = 1;
	msgpack_unpack_uint32_at(o, &v->aid, &i);
	msgpack_unpack_uint32_at(o, &v->iid, &i);
	msgpack_unpack_uint32_at(o, &v->ballot, &i);
}

/**
 * Packs a paxos_repeat structure into a MessagePack buffer using the given packer.
 *
 * @param p Pointer to the MessagePack packer.
 * @param v Pointer to the paxos_repeat structure to be packed.
 */
void msgpack_pack_paxos_repeat(msgpack_packer* p, paxos_repeat* v)
{
	msgpack_pack_array(p, 3);
	msgpack_pack_int32(p, PAXOS_REPEAT);
	msgpack_pack_uint32(p, v->from);
	msgpack_pack_uint32(p, v->to);
}

/**
 * Unpacks a paxos_repeat structure from a MessagePack object.
 *
 * @param o Pointer to the msgpack_object containing the paxos_repeat structure.
 * @param v Pointer to the paxos_repeat structure where the unpacked data will be stored.
 */
void msgpack_unpack_paxos_repeat(msgpack_object* o, paxos_repeat* v)
{
	int i = 1;
	msgpack_unpack_uint32_at(o, &v->from, &i);
	msgpack_unpack_uint32_at(o, &v->to, &i);
}

/**
 * Packs a paxos_trim structure into a MessagePack buffer using the given packer.
 *
 * @param p Pointer to the MessagePack packer.
 * @param v Pointer to the paxos_trim structure to be packed.
 */
void msgpack_pack_paxos_trim(msgpack_packer* p, paxos_trim* v)
{
	msgpack_pack_array(p, 2);
	msgpack_pack_int32(p, PAXOS_TRIM);
	msgpack_pack_uint32(p, v->iid);
}

/**
 * Unpacks a paxos_trim structure from a MessagePack object.
 *
 * @param o Pointer to the msgpack_object containing the paxos_trim structure.
 * @param v Pointer to the paxos_trim structure where the unpacked data will be stored.
 */
void msgpack_unpack_paxos_trim(msgpack_object* o, paxos_trim* v)
{
	int i = 1;
	msgpack_unpack_uint32_at(o, &v->iid, &i);
}

/**
 * Packs a paxos_acceptor_state structure into a MessagePack buffer using the given packer.
 *
 * @param p Pointer to the MessagePack packer.
 * @param v Pointer to the paxos_acceptor_state structure to be packed.
 */
void msgpack_pack_paxos_acceptor_state(msgpack_packer* p, paxos_acceptor_state* v)
{
	msgpack_pack_array(p, 3);
	msgpack_pack_int32(p, PAXOS_ACCEPTOR_STATE);
	msgpack_pack_uint32(p, v->aid);
	msgpack_pack_uint32(p, v->trim_iid);
}

/**
 * Unpacks a paxos_acceptor_state structure from a MessagePack object.
 *
 * @param o Pointer to the msgpack_object containing the paxos_acceptor_state structure.
 * @param v Pointer to the paxos_acceptor_state structure where the unpacked data will be stored.
 */
void msgpack_unpack_paxos_acceptor_state(msgpack_object* o, paxos_acceptor_state* v)
{
	int i = 1;
	msgpack_unpack_uint32_at(o, &v->aid, &i);
	msgpack_unpack_uint32_at(o, &v->trim_iid, &i);
}

/**
 * Packs a paxos_client_value structure into a MessagePack buffer using the given packer.
 *
 * @param p Pointer to the MessagePack packer.
 * @param v Pointer to the paxos_client_value structure to be packed.
 */
void msgpack_pack_paxos_client_value(msgpack_packer* p, paxos_client_value* v)
{
	msgpack_pack_array(p, 2);
	msgpack_pack_int32(p, PAXOS_CLIENT_VALUE);
	msgpack_pack_paxos_value(p, &v->value);
}

/**
 * Unpacks a paxos_client_value structure from a MessagePack object.
 *
 * @param o Pointer to the msgpack_object containing the paxos_client_value structure.
 * @param v Pointer to the paxos_client_value structure where the unpacked data will be stored.
 */
void msgpack_unpack_paxos_client_value(msgpack_object* o, paxos_client_value* v)
{
	int i = 1;
	msgpack_unpack_paxos_value_at(o, &v->value, &i);
}

/**
 * Packs a paxos_message structure into a MessagePack buffer using the given packer.
 * Depending on the type of paxos_message, it calls the corresponding packer function
 * for the appropriate substructure contained within the paxos_message.
 *
 * @param p Pointer to the MessagePack packer.
 * @param v Pointer to the paxos_message structure to be packed.
 */
void msgpack_pack_paxos_message(msgpack_packer* p, paxos_message* v)
{
	switch (v->type) {
	case PAXOS_PREPARE:
		msgpack_pack_paxos_prepare(p, &v->u.prepare);
		break;
	case PAXOS_PROMISE:
		msgpack_pack_paxos_promise(p, &v->u.promise);
		break;
	case PAXOS_ACCEPT:
		msgpack_pack_paxos_accept(p, &v->u.accept);
		break;
	case PAXOS_ACCEPTED:
		msgpack_pack_paxos_accepted(p, &v->u.accepted);
		break;
	case PAXOS_PREEMPTED:
		msgpack_pack_paxos_preempted(p, &v->u.preempted);
		break;
	case PAXOS_REPEAT:
		msgpack_pack_paxos_repeat(p, &v->u.repeat);
		break;
	case PAXOS_TRIM:
		msgpack_pack_paxos_trim(p, &v->u.trim);
		break;
	case PAXOS_ACCEPTOR_STATE:
		msgpack_pack_paxos_acceptor_state(p, &v->u.state);
		break;
	case PAXOS_CLIENT_VALUE:
		msgpack_pack_paxos_client_value(p, &v->u.client_value);
		break;
	}
}

/**
 * Unpacks a paxos_message structure from a MessagePack object.
 * Depending on the type of paxos_message, it calls the corresponding unpacker function
 * for the appropriate substructure contained within the paxos_message.
 *
 * @param o Pointer to the msgpack_object containing the paxos_message structure.
 * @param v Pointer to the paxos_message structure where the unpacked data will be stored.
 */
void msgpack_unpack_paxos_message(msgpack_object* o, paxos_message* v)
{
	v->type = MSGPACK_OBJECT_AT(o,0).u64;
	switch (v->type) {
	case PAXOS_PREPARE:
		msgpack_unpack_paxos_prepare(o, &v->u.prepare);
		break;
	case PAXOS_PROMISE:
		msgpack_unpack_paxos_promise(o, &v->u.promise);
		break;
	case PAXOS_ACCEPT:
		msgpack_unpack_paxos_accept(o, &v->u.accept);
		break;
	case PAXOS_ACCEPTED:
		msgpack_unpack_paxos_accepted(o, &v->u.accepted);
		break;
	case PAXOS_PREEMPTED:
		msgpack_unpack_paxos_preempted(o, &v->u.preempted);
		break;
	case PAXOS_REPEAT:
		msgpack_unpack_paxos_repeat(o, &v->u.repeat);
		break;
	case PAXOS_TRIM:
		msgpack_unpack_paxos_trim(o, &v->u.trim);
		break;
	case PAXOS_ACCEPTOR_STATE:
		msgpack_unpack_paxos_acceptor_state(o, &v->u.state);
		break;
	case PAXOS_CLIENT_VALUE:
		msgpack_unpack_paxos_client_value(o, &v->u.client_value);
		break;
	}
}
