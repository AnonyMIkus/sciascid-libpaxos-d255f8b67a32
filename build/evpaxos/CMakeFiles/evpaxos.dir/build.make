# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build"

# Include any dependencies generated for this target.
include evpaxos/CMakeFiles/evpaxos.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include evpaxos/CMakeFiles/evpaxos.dir/compiler_depend.make

# Include the progress variables for this target.
include evpaxos/CMakeFiles/evpaxos.dir/progress.make

# Include the compile flags for this target's objects.
include evpaxos/CMakeFiles/evpaxos.dir/flags.make

evpaxos/CMakeFiles/evpaxos.dir/config.c.o: evpaxos/CMakeFiles/evpaxos.dir/flags.make
evpaxos/CMakeFiles/evpaxos.dir/config.c.o: ../evpaxos/config.c
evpaxos/CMakeFiles/evpaxos.dir/config.c.o: evpaxos/CMakeFiles/evpaxos.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building C object evpaxos/CMakeFiles/evpaxos.dir/config.c.o"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT evpaxos/CMakeFiles/evpaxos.dir/config.c.o -MF CMakeFiles/evpaxos.dir/config.c.o.d -o CMakeFiles/evpaxos.dir/config.c.o -c "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/config.c"

evpaxos/CMakeFiles/evpaxos.dir/config.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evpaxos.dir/config.c.i"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/config.c" > CMakeFiles/evpaxos.dir/config.c.i

evpaxos/CMakeFiles/evpaxos.dir/config.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evpaxos.dir/config.c.s"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/config.c" -o CMakeFiles/evpaxos.dir/config.c.s

evpaxos/CMakeFiles/evpaxos.dir/message.c.o: evpaxos/CMakeFiles/evpaxos.dir/flags.make
evpaxos/CMakeFiles/evpaxos.dir/message.c.o: ../evpaxos/message.c
evpaxos/CMakeFiles/evpaxos.dir/message.c.o: evpaxos/CMakeFiles/evpaxos.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Building C object evpaxos/CMakeFiles/evpaxos.dir/message.c.o"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT evpaxos/CMakeFiles/evpaxos.dir/message.c.o -MF CMakeFiles/evpaxos.dir/message.c.o.d -o CMakeFiles/evpaxos.dir/message.c.o -c "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/message.c"

evpaxos/CMakeFiles/evpaxos.dir/message.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evpaxos.dir/message.c.i"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/message.c" > CMakeFiles/evpaxos.dir/message.c.i

evpaxos/CMakeFiles/evpaxos.dir/message.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evpaxos.dir/message.c.s"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/message.c" -o CMakeFiles/evpaxos.dir/message.c.s

evpaxos/CMakeFiles/evpaxos.dir/paxos_types_pack.c.o: evpaxos/CMakeFiles/evpaxos.dir/flags.make
evpaxos/CMakeFiles/evpaxos.dir/paxos_types_pack.c.o: ../evpaxos/paxos_types_pack.c
evpaxos/CMakeFiles/evpaxos.dir/paxos_types_pack.c.o: evpaxos/CMakeFiles/evpaxos.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_3) "Building C object evpaxos/CMakeFiles/evpaxos.dir/paxos_types_pack.c.o"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT evpaxos/CMakeFiles/evpaxos.dir/paxos_types_pack.c.o -MF CMakeFiles/evpaxos.dir/paxos_types_pack.c.o.d -o CMakeFiles/evpaxos.dir/paxos_types_pack.c.o -c "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/paxos_types_pack.c"

evpaxos/CMakeFiles/evpaxos.dir/paxos_types_pack.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evpaxos.dir/paxos_types_pack.c.i"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/paxos_types_pack.c" > CMakeFiles/evpaxos.dir/paxos_types_pack.c.i

evpaxos/CMakeFiles/evpaxos.dir/paxos_types_pack.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evpaxos.dir/paxos_types_pack.c.s"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/paxos_types_pack.c" -o CMakeFiles/evpaxos.dir/paxos_types_pack.c.s

evpaxos/CMakeFiles/evpaxos.dir/peers.c.o: evpaxos/CMakeFiles/evpaxos.dir/flags.make
evpaxos/CMakeFiles/evpaxos.dir/peers.c.o: ../evpaxos/peers.c
evpaxos/CMakeFiles/evpaxos.dir/peers.c.o: evpaxos/CMakeFiles/evpaxos.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_4) "Building C object evpaxos/CMakeFiles/evpaxos.dir/peers.c.o"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT evpaxos/CMakeFiles/evpaxos.dir/peers.c.o -MF CMakeFiles/evpaxos.dir/peers.c.o.d -o CMakeFiles/evpaxos.dir/peers.c.o -c "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/peers.c"

evpaxos/CMakeFiles/evpaxos.dir/peers.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evpaxos.dir/peers.c.i"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/peers.c" > CMakeFiles/evpaxos.dir/peers.c.i

evpaxos/CMakeFiles/evpaxos.dir/peers.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evpaxos.dir/peers.c.s"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/peers.c" -o CMakeFiles/evpaxos.dir/peers.c.s

evpaxos/CMakeFiles/evpaxos.dir/evacceptor.c.o: evpaxos/CMakeFiles/evpaxos.dir/flags.make
evpaxos/CMakeFiles/evpaxos.dir/evacceptor.c.o: ../evpaxos/evacceptor.c
evpaxos/CMakeFiles/evpaxos.dir/evacceptor.c.o: evpaxos/CMakeFiles/evpaxos.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_5) "Building C object evpaxos/CMakeFiles/evpaxos.dir/evacceptor.c.o"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT evpaxos/CMakeFiles/evpaxos.dir/evacceptor.c.o -MF CMakeFiles/evpaxos.dir/evacceptor.c.o.d -o CMakeFiles/evpaxos.dir/evacceptor.c.o -c "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/evacceptor.c"

evpaxos/CMakeFiles/evpaxos.dir/evacceptor.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evpaxos.dir/evacceptor.c.i"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/evacceptor.c" > CMakeFiles/evpaxos.dir/evacceptor.c.i

evpaxos/CMakeFiles/evpaxos.dir/evacceptor.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evpaxos.dir/evacceptor.c.s"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/evacceptor.c" -o CMakeFiles/evpaxos.dir/evacceptor.c.s

evpaxos/CMakeFiles/evpaxos.dir/evlearner.c.o: evpaxos/CMakeFiles/evpaxos.dir/flags.make
evpaxos/CMakeFiles/evpaxos.dir/evlearner.c.o: ../evpaxos/evlearner.c
evpaxos/CMakeFiles/evpaxos.dir/evlearner.c.o: evpaxos/CMakeFiles/evpaxos.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_6) "Building C object evpaxos/CMakeFiles/evpaxos.dir/evlearner.c.o"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT evpaxos/CMakeFiles/evpaxos.dir/evlearner.c.o -MF CMakeFiles/evpaxos.dir/evlearner.c.o.d -o CMakeFiles/evpaxos.dir/evlearner.c.o -c "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/evlearner.c"

evpaxos/CMakeFiles/evpaxos.dir/evlearner.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evpaxos.dir/evlearner.c.i"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/evlearner.c" > CMakeFiles/evpaxos.dir/evlearner.c.i

evpaxos/CMakeFiles/evpaxos.dir/evlearner.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evpaxos.dir/evlearner.c.s"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/evlearner.c" -o CMakeFiles/evpaxos.dir/evlearner.c.s

evpaxos/CMakeFiles/evpaxos.dir/evproposer.c.o: evpaxos/CMakeFiles/evpaxos.dir/flags.make
evpaxos/CMakeFiles/evpaxos.dir/evproposer.c.o: ../evpaxos/evproposer.c
evpaxos/CMakeFiles/evpaxos.dir/evproposer.c.o: evpaxos/CMakeFiles/evpaxos.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_7) "Building C object evpaxos/CMakeFiles/evpaxos.dir/evproposer.c.o"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT evpaxos/CMakeFiles/evpaxos.dir/evproposer.c.o -MF CMakeFiles/evpaxos.dir/evproposer.c.o.d -o CMakeFiles/evpaxos.dir/evproposer.c.o -c "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/evproposer.c"

evpaxos/CMakeFiles/evpaxos.dir/evproposer.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evpaxos.dir/evproposer.c.i"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/evproposer.c" > CMakeFiles/evpaxos.dir/evproposer.c.i

evpaxos/CMakeFiles/evpaxos.dir/evproposer.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evpaxos.dir/evproposer.c.s"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/evproposer.c" -o CMakeFiles/evpaxos.dir/evproposer.c.s

evpaxos/CMakeFiles/evpaxos.dir/evreplica.c.o: evpaxos/CMakeFiles/evpaxos.dir/flags.make
evpaxos/CMakeFiles/evpaxos.dir/evreplica.c.o: ../evpaxos/evreplica.c
evpaxos/CMakeFiles/evpaxos.dir/evreplica.c.o: evpaxos/CMakeFiles/evpaxos.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_8) "Building C object evpaxos/CMakeFiles/evpaxos.dir/evreplica.c.o"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT evpaxos/CMakeFiles/evpaxos.dir/evreplica.c.o -MF CMakeFiles/evpaxos.dir/evreplica.c.o.d -o CMakeFiles/evpaxos.dir/evreplica.c.o -c "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/evreplica.c"

evpaxos/CMakeFiles/evpaxos.dir/evreplica.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evpaxos.dir/evreplica.c.i"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/evreplica.c" > CMakeFiles/evpaxos.dir/evreplica.c.i

evpaxos/CMakeFiles/evpaxos.dir/evreplica.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evpaxos.dir/evreplica.c.s"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos/evreplica.c" -o CMakeFiles/evpaxos.dir/evreplica.c.s

# Object files for target evpaxos
evpaxos_OBJECTS = \
"CMakeFiles/evpaxos.dir/config.c.o" \
"CMakeFiles/evpaxos.dir/message.c.o" \
"CMakeFiles/evpaxos.dir/paxos_types_pack.c.o" \
"CMakeFiles/evpaxos.dir/peers.c.o" \
"CMakeFiles/evpaxos.dir/evacceptor.c.o" \
"CMakeFiles/evpaxos.dir/evlearner.c.o" \
"CMakeFiles/evpaxos.dir/evproposer.c.o" \
"CMakeFiles/evpaxos.dir/evreplica.c.o"

# External object files for target evpaxos
evpaxos_EXTERNAL_OBJECTS =

evpaxos/libevpaxos.so: evpaxos/CMakeFiles/evpaxos.dir/config.c.o
evpaxos/libevpaxos.so: evpaxos/CMakeFiles/evpaxos.dir/message.c.o
evpaxos/libevpaxos.so: evpaxos/CMakeFiles/evpaxos.dir/paxos_types_pack.c.o
evpaxos/libevpaxos.so: evpaxos/CMakeFiles/evpaxos.dir/peers.c.o
evpaxos/libevpaxos.so: evpaxos/CMakeFiles/evpaxos.dir/evacceptor.c.o
evpaxos/libevpaxos.so: evpaxos/CMakeFiles/evpaxos.dir/evlearner.c.o
evpaxos/libevpaxos.so: evpaxos/CMakeFiles/evpaxos.dir/evproposer.c.o
evpaxos/libevpaxos.so: evpaxos/CMakeFiles/evpaxos.dir/evreplica.c.o
evpaxos/libevpaxos.so: evpaxos/CMakeFiles/evpaxos.dir/build.make
evpaxos/libevpaxos.so: paxos/libpaxos.a
evpaxos/libevpaxos.so: /usr/lib/x86_64-linux-gnu/libevent.so
evpaxos/libevpaxos.so: /usr/lib/x86_64-linux-gnu/libmsgpackc.so
evpaxos/libevpaxos.so: evpaxos/CMakeFiles/evpaxos.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir="/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_9) "Linking C shared library libevpaxos.so"
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/evpaxos.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
evpaxos/CMakeFiles/evpaxos.dir/build: evpaxos/libevpaxos.so
.PHONY : evpaxos/CMakeFiles/evpaxos.dir/build

evpaxos/CMakeFiles/evpaxos.dir/clean:
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" && $(CMAKE_COMMAND) -P CMakeFiles/evpaxos.dir/cmake_clean.cmake
.PHONY : evpaxos/CMakeFiles/evpaxos.dir/clean

evpaxos/CMakeFiles/evpaxos.dir/depend:
	cd "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32" "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/evpaxos" "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build" "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos" "/mnt/c/Users/Obj/Desktop/Praxis + Bachelor Versioniert/V2/sciascid-libpaxos-d255f8b67a32/sciascid-libpaxos-d255f8b67a32/build/evpaxos/CMakeFiles/evpaxos.dir/DependInfo.cmake" --color=$(COLOR)
.PHONY : evpaxos/CMakeFiles/evpaxos.dir/depend

