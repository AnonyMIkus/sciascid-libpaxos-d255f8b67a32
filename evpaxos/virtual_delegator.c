/*
* The virtual delegator should be running between replica and other delegator.
* Therefor it is necessary to maintain a runnable accesspoint for replica.
* It needs a connection (via peers) to a group of replica and make use of evdelegator.
*/

struct virtual_delegator {
	struct peers parent;
	struct peers* delegators;
	struct peers* peers_group;
};

void add_to_group(struct peers add) {}

void del_from_group() {}

void sending_to_all() {}

void hearthbeat() {}

