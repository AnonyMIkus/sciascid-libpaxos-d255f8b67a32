#include "gtest/gtest.h"
#include "evpaxos.h"
#include "test_client.h"
#include "replica_thread.h"
#include <iostream>
#include <fstream>

using std::ofstream;

int start_replicas_from_config2(const char* config_file,
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
}