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


#include "paxos.h"
#include "message.h"
#include "paxos_types_pack.h"
#include <string.h>

 /**
  * Callback function to pack data and write it to a bufferevent.
  *
  * @param data A pointer to the bufferevent where the data should be written.
  * @param buf A pointer to the data buffer to be written.
  * @param len The length of the data to be written.
  * @return Always returns 0.
  */
static int bufferevent_pack_data(void* data, const char* buf, size_t len)
{
	struct bufferevent* bev = (struct bufferevent*)data;
	bufferevent_write(bev, buf, len);
	return 0;
}

/**
 * Packs and sends a Paxos message using a bufferevent.
 *
 * @param bev The bufferevent to use for sending the message.
 * @param msg A pointer to the Paxos message to be sent.
 */
void send_paxos_message(struct bufferevent* bev, paxos_message* msg)
{
	msgpack_packer* packer;
	// Create a msgpack_packer associated with the provided bufferevent
	packer = msgpack_packer_new(bev, bufferevent_pack_data);
	// Pack the paxos_message using the msgpack_packer
	msgpack_pack_paxos_message(packer, msg);
	msgpack_packer_free(packer);
}

/**
 * Sends a Paxos prepare message using a bufferevent.
 *
 * @param bev The bufferevent to use for sending the prepare message.
 * @param p A pointer to the Paxos prepare message to be sent.
 */
void send_paxos_prepare(struct bufferevent* bev, paxos_prepare* p)
{
	paxos_message msg = {
		.type = PAXOS_PREPARE,
		.u.prepare = *p };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send prepare for iid %d ballot %d", p->iid, p->ballot);
}

/**
 * Sends a Paxos promise message using a bufferevent.
 *
 * @param bev The bufferevent to use for sending the promise message.
 * @param p A pointer to the Paxos promise message to be sent.
 */
void send_paxos_promise(struct bufferevent* bev, paxos_promise* p)
{
	paxos_message msg = {
		.type = PAXOS_PROMISE,
		.u.promise = *p };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send promise for iid %d ballot %d", p->iid, p->ballot);
}

/**
 * Packs and sends a Paxos accept message using a bufferevent.
 *
 * @param bev Pointer to the bufferevent where the packed message will be sent.
 * @param p Pointer to the paxos_accept structure to be packed and sent.
 */
void send_paxos_accept(struct bufferevent* bev, paxos_accept* p)
{
	paxos_message msg = {
		.type = PAXOS_ACCEPT,
		.u.accept = *p };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send accept for iid %d ballot %d", p->iid, p->ballot);
}

/**
 * Packs and sends a Paxos accepted message using a bufferevent.
 *
 * @param bev Pointer to the bufferevent where the packed message will be sent.
 * @param p Pointer to the paxos_accepted structure to be packed and sent.
 */
void send_paxos_accepted(struct bufferevent* bev, paxos_accepted* p)
{	
	paxos_message msg = {
		.type = PAXOS_ACCEPTED,
		.u.accepted = *p };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send accepted for inst %d ballot %d", p->iid, p->ballot);
}


/**
 * Packs and sends a Paxos preempted message using a bufferevent.
 *
 * @param bev Pointer to the bufferevent where the packed message will be sent.
 * @param p Pointer to the paxos_preempted structure to be packed and sent.
 */
void send_paxos_preempted(struct bufferevent* bev, paxos_preempted* p)
{
	paxos_message msg = {
		.type = PAXOS_PREEMPTED,
		.u.preempted = *p };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send preempted for inst %d ballot %d", p->iid, p->ballot);
}

/**
 * Packs and sends a Paxos repeat message using a bufferevent.
 *
 * @param bev Pointer to the bufferevent where the packed message will be sent.
 * @param p Pointer to the paxos_repeat structure to be packed and sent.
 */
void send_paxos_repeat(struct bufferevent* bev, paxos_repeat* p)
{
	paxos_message msg = {
		.type = PAXOS_REPEAT,
		.u.repeat = *p };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send repeat for inst %d-%d", p->from, p->to);
}


/**
 * Packs and sends a Paxos trim message using a bufferevent.
 *
 * @param bev Pointer to the bufferevent where the packed message will be sent.
 * @param t Pointer to the paxos_trim structure to be packed and sent.
 */
void send_paxos_trim(struct bufferevent* bev, paxos_trim* t)
{
	paxos_message msg = {
		.type = PAXOS_TRIM,
		.u.trim = *t };
	send_paxos_message(bev, &msg);
	paxos_log_debug("Send trim for inst %d", t->iid);
}

/**
 * Packs and sends a client value submission message using a bufferevent.
 *
 * @param bev Pointer to the bufferevent where the packed message will be sent.
 * @param data Pointer to the data of the client value.
 * @param size Size of the client value data.
 */
void paxos_submit(struct bufferevent* bev, char* data, int size)
{
	paxos_message msg = {
		.type = PAXOS_CLIENT_VALUE,
		.u.client_value.value.paxos_value_len = size,
		.u.client_value.value.paxos_value_val = data };
	send_paxos_message(bev, &msg);
}

/**
 * recv_paxos_message
 *
 * Unpacks a Paxos message from an event buffer and stores the result in the provided paxos_message structure.
 *
 * @param in Pointer to the input event buffer containing the packed Paxos message.
 * @param out Pointer to the paxos_message structure where the unpacked message will be stored.
 * @return Returns 1 if the message was successfully unpacked and stored, 0 otherwise.
 */
int recv_paxos_message(struct evbuffer* in, paxos_message* out)
{
	int rv = 0; // Return value indicating successful unpacking
	char* buffer; // Buffer holding packed message data
	size_t size, offset = 0; // Size of packed data and offset for unpacking
	msgpack_unpacked msg; // Unpacked message structure
	
	size = evbuffer_get_length(in); // Get the size of data in the input buffer
	if (size == 0) // Return 0 if buffer is empty
		return rv;
	
	msgpack_unpacked_init(&msg); // Initialize the unpacked message structure
	buffer = (char*)evbuffer_pullup(in, size);	// Get the pointer to the data in the buffer
	if (msgpack_unpack_next(&msg, buffer, size, &offset)) {
		msgpack_unpack_paxos_message(&msg.data, out); // Unpack the message into the provided paxos_message structure
		evbuffer_drain(in, offset); // Remove the consumed data from the input buffer
		rv = 1;
	}
	msgpack_unpacked_destroy(&msg);  // Clean up the unpacked message structure
	return rv; // Return the result of the unpacking process (1 for success, 0 for failure)
}
