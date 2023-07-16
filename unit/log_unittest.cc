#include "gtest/gtest.h"
#include "evpaxos.h"
#include "test_client.h"
#include "replica_thread.h"
#include <iostream>
#include <fstream>

using std::ofstream;

/*int start_replicas_from_config2(const char* config_file,
	struct replica_thread** threads, int deliveries)
{
	int i;
	struct evpaxos_config* config = evpaxos_config_read(config_file);
	int count = evpaxos_acceptor_count(config);
	*threads = (replica_thread*)calloc(count, sizeof(struct replica_thread));
	for (i = 0; i < count; i++)
		replica_thread_create(&(*threads)[i], i, config_file, deliveries);
	evpaxos_config_free(config);
	return count;
}

TEST(LogTest, TotalOrderDeliveryLogTest) {
	struct replica_thread* threads;
	int i, j, replicas, deliveries = 100000;

	replicas = start_replicas_from_config2("config/replicas.conf",
		&threads, deliveries);
	test_client* client = test_client_new("config/replicas.conf", 0);
	GTEST_LOG_(INFO) << "replicas: " << replicas;
	ofstream logfile;
	logfile.open("TestLogFile.txt");
	logfile << "replicas: " << replicas << "\n";
	logfile.close();

	for (i = 0; i < deliveries; i++)
		test_client_submit_value(client, i);

	int* values[replicas];
	for (i = 0; i < replicas; i++)
		values[i] = replica_thread_wait_deliveries(&threads[i]);

	for (i = 0; i < replicas; i++)
		for (j = 0; j < deliveries; j++)
			ASSERT_EQ(values[i][j], j);

	test_client_free(client);
	for (i = 0; i < replicas; i++)
		replica_thread_destroy(&threads[i]);
}*/

#include "acceptor.h"
#include "gtest/gtest.h"
#include <stdio.h>

class AcceptorTest : public::testing::TestWithParam<paxos_storage_backend> {
protected:

	int id;
	struct acceptor* a;

	virtual void SetUp() {
		id = 2;
		paxos_config.verbosity = PAXOS_LOG_QUIET;
		paxos_config.storage_backend = GetParam();
		paxos_config.trash_files = 1;
		a = acceptor_new(id);
	}

	virtual void TearDown() {
		acceptor_free(a);
	}
};

#define CHECK_PROMISE(msg, id, bal, vbal, val) {            \
	ASSERT_EQ(msg.type, PAXOS_PROMISE);                     \
	ASSERT_EQ(msg.u.promise.iid, id);                       \
	ASSERT_EQ(msg.u.promise.ballot, bal);                   \
	ASSERT_EQ(msg.u.promise.value_ballot, vbal);            \
	ASSERT_EQ(msg.u.promise.value.paxos_value_len,          \
		val == NULL ? 0 : strlen(val)+1);                   \
	ASSERT_STREQ(msg.u.promise.value.paxos_value_val, val); \
}

#define CHECK_ACCEPTED(msg, id, bal, vbal, val) {            \
	ASSERT_EQ(msg.type, PAXOS_ACCEPTED);                     \
	ASSERT_EQ(msg.u.accepted.iid, id);                       \
	ASSERT_EQ(msg.u.accepted.ballot, bal);                   \
	ASSERT_EQ(msg.u.accepted.value_ballot, vbal);            \
	ASSERT_EQ(msg.u.accepted.value.paxos_value_len,          \
		val == NULL ? 0 : strlen(val)+1);                    \
	ASSERT_STREQ(msg.u.accepted.value.paxos_value_val, val); \
}

#define CHECK_PREEMPTED(msg, id, bal) {     \
	ASSERT_EQ(msg.type, PAXOS_PREEMPTED);   \
	ASSERT_EQ(msg.u.preempted.iid, id);     \
	ASSERT_EQ(msg.u.preempted.ballot, bal); \
}

TEST_P(AcceptorTest, Prepare2) {
	int counter = 0;
	paxos_message msg;
	paxos_prepare pre = { 1, 101 };
	acceptor_receive_prepare(a, &pre, &msg);
	CHECK_PROMISE(msg, 1, 101, 0, NULL);
	counter++;
	printf("%d\n", counter);
	ofstream logfile;
	logfile.open("TestLogFile.txt");
	logfile << "counted: " << counter << "\n";
	logfile.close();
}