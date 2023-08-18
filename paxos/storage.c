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


#include "storage.h"
#include <stdlib.h>

/// <summary>
/// Initialize storage for an Acceptor and checking which set up has to be chosen for allocating.
/// </summary>
/// <param name="store">Reference  to dereferenced storage that is inside of current Acceptor.</param>
/// <param name="acceptor_id">Id of current Acceptor</param>
void storage_init(struct storage* store, int acceptor_id)
{
	switch(paxos_config.storage_backend) { // Checking which storage is accessible.
		case PAXOS_MEM_STORAGE: // Storage works in Memory. Use of storage_mem.c.
			storage_init_mem(store, acceptor_id); 
			break;
		#ifdef HAS_LMDB // Storage works with LMDB. Use of storage_lmdb.c.
		case PAXOS_LMDB_STORAGE:
			storage_init_lmdb(store, acceptor_id);
			break;
		#endif
		default: // Error Code
		paxos_log_error("Storage backend not available");
		exit(0);
	}
}

/*
* Here all functions are assigned and refering to storage_mem or storage_lmdb (depending on storage_init).
* The usage of word 'handle' can be a little bit cofusing. 
* It is a reference to struct mem_storage (in storage_memory file) or struct lmdb_storage (in storage_lmdb file).
* If a call with the reference to struct storage with variable api was made then handle give the correct reference to work with.
* 
* Example:
*	Assumption: Storage is working in memory and we created an instance of storage_mem.
*	That means handle has a reference to a struct mem_storage and functions are assign to function in storage_mem.
*	If we use storage_close, it can call mem_storage_close with handle who is using a correct reference of struct mem_storage.
*	The allocated memory will be freed.
*/
int storage_open(struct storage* store)
{
	return store->api.open(store->handle);
}

void storage_close(struct storage* store)
{
	store->api.close(store->handle);
}

int storage_tx_begin(struct storage* store)
{
	return store->api.tx_begin(store->handle);
}

int storage_tx_commit(struct storage* store)
{
	return store->api.tx_commit(store->handle);
}

void
storage_tx_abort(struct storage* store)
{
	store->api.tx_abort(store->handle);
}

int
storage_get_record(struct storage* store, iid_t iid, paxos_accepted* out)
{
	return store->api.get(store->handle, iid, out);
}

int
storage_put_record(struct storage* store, paxos_accepted* acc)
{
	return store->api.put(store->handle, acc);
}

int
storage_trim(struct storage* store, iid_t iid)
{
	return store->api.trim(store->handle, iid);
}

iid_t storage_get_trim_instance(struct storage* store)
{
	return store->api.get_trim_instance(store->handle);
}
