# A manual of libpaxos

## Manual

### Table of Content

- [A manual of libpaxos](#a-manual-of-libpaxos)
  - [Manual](#manual)
    - [Table of Content](#table-of-content)
      - [Requirement](#requirement)
      - [Visual Studio 2022](#visual-studio-2022)
      - [libpaxos3](#libpaxos3)
      - [WSL 2](#wsl-2)
      - [CMake](#cmake)
      - [Libraries, Build \& Test](#libraries-build--test)
      - [Run program](#run-program)
      - [paxos.conf](#paxosconf)

 
#### <a id='req'>Requirement</a>

This is just a list and the installation of all thi requirement will be after this chapter.

 - Visual Studio 2022 Community (VS 2022)
 - [libpaxos](https://libpaxos.sourceforge.net/)
 - WSL 2 on Windows
 - CMake in WSL 2
 - [libevent](https://libevent.org/)
 - [msgpack](https://msgpack.org/)
 - (optional) LMDB

 In this project there is no LMDB included. That means 

#### <a id='vs'>Visual Studio 2022</a>

Here is a small guide to install VS 2022:

https://github.com/MicrosoftDocs/cpp-docs/blob/main/docs/build/vscpp-step-0-installation.md

From here we work with VS 2022.

#### <a id='paxos'>libpaxos<sup>3</sup></a>

In requirements I linked the page to libpaxos. You will land on a page with multiple versions of Paxos. 
Follow the link to [libpaxos<sup>3</sup>](https://libpaxos.sourceforge.net/paxos_projects.php#libpaxos3).
You can either use git-command in terminal to download or with clone-button when you follow the link to bitbucket.
The file will a similar name that begings with sciascid-libpaxos-< combination of number and letters >
Open folder with VS 2022.

#### <a id='wsl'>WSL 2</a>

You start with an opened VS 2022.

> :exclamation: Note
>
> WSL stands for Windows-Subsystem for Linux. IN VS 2022 there is an option to open an terminal. In the menu "View" there is a point named terminal. The terminal will be opened at the bottom side of VS 2022.


If you do not have WSL installed you can do it with the following command in a powershell:

```powershell
PS currentPath> wsl --install
```

If you have already WSL installed but you are not sure if you have the newest version then you can update with following command:

```powershell
PS currentPath> wsl --update
```

For this project I am using Ubuntu as system for WSL. To install Ubuntu you execute following command:

```powershell
PS currentPath> wsl --install -d Ubuntu
```

 WSL will guide you through download and install a Long-Term Supported (LTS) Version of Ubuntu. Currently I am using Ubuntu 22.04.
 After installing Ubuntu you have to type an username and password. There are some rules to username:
 
 - username must be in lowercase letters
 - underscore ("_") and dash ("-") is allowed

 > :warning: **Small warning for password**
 >
 > You will not see anything when typing a password.
 > Type password as you would normally do and memorize it in some way.

 #### <a id='cmake'>CMake</a>

 If you installed Ubuntu from the previous step then VS 2022 is switching to WSL automatically.

 But you can switch to WSL manually at any time like this:

 ```powershell
 PS currentPath> wsl
 ```

 Switch to libpaxos folder with cd command. You will find it under **``/mnt/< drive - letter >/ < path of libpaxos in windows >``** (We will keep the path short with pathToLibpaxos).
 Disk-drivers are mounted in WSL per default and can be found in path **``/mnt``**.
 
 Befor we can work on libpaxos we need to install some libraries.
 It will look similar this:
 ```bash
 username@system:/pathToLibpaxos$
 ```

 Following steps are needed:

 1. First we get CMake first:
  ```bash
 username@system:/pathToLibpaxos$ sudo apt-get install cmake
 ```

 2. To get gcc and other essential tools that round up CMake:
 ```bash
 username@system:/pathToLibpaxos$ sudo apt-get install build-essential
 ```

 #### <a id='rest'>Libraries, Build & Test</a>

 We still need libraries befor we can build or do anything else.
 libevent, msgpack and gtest are needed for that:

 ```bash
 username@system:/pathToLibpaxos$ sudo apt-get install libevent-dev
 username@system:/pathToLibpaxos$ sudo apt-get install libmsgpack-dev
 username@system:/pathToLibpaxos$ sudo apt-get install libgtest-dev
 ```

 CMake will find this libraries in the process. Now we can build up the project.
 For that create a folder build and switch to it:

 ```bash
 username@system:/pathToLibpaxos$ mkdir build
 username@system:/pathToLibpaxos$ cd build
 ```

Alternativ:

```bash
username@system:/pathToLibpaxos$ mkdir build && cd build
```

This structure will make the process easier to work with.
From here the next step would be to build.

```bash
username@system:/pathToLibpaxos/build$ cmake ..
username@system:/pathToLibpaxos/build$ make
```

You can testing test (that are in folder unit) with following command:

```bash
username@system:/pathToLibpaxos/build$ ctest -VV
```

```-VV``` allows an extra verbos output of the test.

#### Run program

Inside the sample folder, you'll find multiple main functions. The files within this folder are already complete and can be executed within WSL (Windows Subsystem for Linux). To ensure proper functionality, three replicas are required to be running. Once the replicas are active, clients can begin sending proposals.

Each replica serves as a proposer, acceptor, and learner simultaneously. This means that every replica is capable of initiating and transmitting proposals within the group.

These function calls require the file [paxos.conf](#paxosconf). More information about this file can be found in the chapter below.

The following commands demonstrate how to run them using WSL:

1. **replica2**

There is a file called `replica2`. This file reads information such as replicas and proposer details from the given [conf](#paxosconf)-file and initiates their execution.

**Structure**

```bash
username@system:/pathToLibpaxos/build$ ./sampl/replica2 pathToPaxosConf
```

**Example:**

```bash
username@system:/pathToLibpaxos/build$ ./sampl/replica2 ../paxos.conf
```

The are no other rules for the name of the conf-file than of the OS. It just have to be on the previous level of the file structure. 

```bash
username@system:/pathToLibpaxos/build$ ./sampl/replica2 ../basic5.conf
```

2. **client**

**Possible structures**

```bash
username@system:/pathToLibpaxos/build$ ./sampl/replica pathToPaxosConf -o # -v # -p #
username@system:/pathToLibpaxos/build$ ./sampl/replica pathToPaxosConf --outstanding # --value-size # --proposer-id #
```

For better understanding:
- ``-o`` or ``--outstanding``: _"Number of outstanding client values"_
- ``-v`` or ``--value-size``: _"Size of client value (in bytes)"_
- ``-p`` or ``proposer-id``: _"ID of the proposer to connect to"_

**Example:**

```bash
username@system:/pathToLibpaxos/build$ ./sampl/client ../paxos.conf -o 100 -v 200 -p 0
```

For the name of the conf-file: The rules are the same as for replica2.

#### paxos.conf

The `paxos.conf` file holds significant importance for the emulator. This file provides essential information such as IP addresses, ports, and more for replicas, proposers, and other components. The '#' sign functions as a comment indicator. As a result, the text that is not marked as comment is read and processed.

```
## LibPaxos configuration file
# Specify an id, ip address and port for each replica.
# Ids must start from 0 and must be unique.
# Uncommented text works like an array. That means if you call a main from sample with an id then you call element from here on position that is the same like id.

replica 0 127.0.0.1 8800
replica 1 127.0.0.1 8801
replica 2 127.0.0.1 8802
#replica 3 127.0.0.1 8803
#replica 4 127.0.0.1 8804
#replica 5 127.0.0.1 8805
# Alternatively it is possible to specify acceptors and proposers separately.
#acceptor 0 127.0.0.1 8809
#acceptor 1 127.0.0.1 8810
#acceptor 2 127.0.0.1 8811
#proposer 0 127.0.0.1 8806
#proposer 1 127.0.0.1 8807
#proposer 2 127.0.0.1 8808
# Verbosity level: must be one of quiet, error, info, or debug.
# Default is info.
verbosity debug
# Enable TCP_NODELAY?
# Default is 'yes'.
# tcp-nodelay no
################################### Learners ##################################
# Should learners start from instance 0 when starting up?
# Default is 'yes'.
# learner-catch-up no
################################## Proposers ##################################
# How many seconds should pass before a proposer times out an instance?
# Default is 1.
# proposer-timeout 10
# How many phase 1 instances should proposers preexecute?
# Default is 128.
# proposer-preexec-window 1024
################################## Acceptors ##################################
# Acceptor storage backend: must be one of memory or lmdb.
# Default is memory.
# storage-backend lmdb
# Should the acceptor trash previous storage files and start from scratch?
# This is here only for testing purposes.
# Default is 'no'.
# acceptor-trash-files yes
############################ LMDB acceptor storage ############################
# Should lmdb write to disk synchronously?
# Default is 'no'.
# lmdb-sync yes
# Path for lmdb database environment.
# lmdb-env-path /tmp/acceptor
# lmdb's map size in bytes (maximum size of the database).
# Accepted units are mb, kb and gb.
# Default is 10mb.
# lmdb-mapsize 1gb
```