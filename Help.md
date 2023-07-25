## Start program

Elements in paxos.conf are behaving like an array.

replica
```bash
user@pathToPaxos\build> ./sample/replica 0 ../paxos.config
```

Call of replica on position 0 in paxos.conf.

replica
```bash
user@pathToPaxos\build> ./sample/replica entry_positon_as_int ../paxos.config
```

proposer
```bash
user@pathToPaxos\build> ./sample/proposer entry_positon_as_int ../paxos.config
```

client
```bash
user@pathToPaxos\build> ./sample/client ../paxos.config -o not_0_int_value -v value_sie_and_not_0_int_value -p id_of_proposer_and_not_0_int_value
```