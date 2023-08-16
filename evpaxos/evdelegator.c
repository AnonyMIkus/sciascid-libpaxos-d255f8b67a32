#include "include/evdelegator.h"

struct evdelegator {
	int id;
	int maxAcc;
	int currentAcc;
	uint32_t* aids;
	struct peers* peers;
	struct quorum* quorum;
};

static void send_quorum() 
{

}