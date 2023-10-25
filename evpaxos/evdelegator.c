#include "include/evdelegator.h"

/*
* Evdelegator - A network based implementation of delegator.
* 
* We need information to be saved and known through a struct (struct evdelegator).
*
* The functions add_to_group() and del_from_group() should allow to add/remove acceptors from the known group if needed.
* Evdelegator needs to send propose and accept messages to other replicas.
* Getting parameters via virtual_delegator.
*/

struct evdelegator {
	int id;
	struct evacceptor* delegators;
	uint32_t* aids;
	struct quorum* sub_quorum;
};

struct evdelegator* evdelegator_init(int given_id, int groupid) {}

void add_to_group(struct evacceptor* add) {}

void del_from_group(struct evacceptor* add) {}

void relegate_data() {}