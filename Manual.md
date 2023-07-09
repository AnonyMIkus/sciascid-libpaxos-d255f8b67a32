# A manual of libpaxos

## Manual

### Table of Content

 - [Requirement](#Requirement)
 - [WSL](#WSL)

#### Requirement

This is just a list and the installation of all thi requirement will be after this chapter.

 - Visual Studio 2022 Community (VS 2022)
 - [libpaxos](https://libpaxos.sourceforge.net/)
 - WSL 2 on Windows
 - CMake in WSL 2
 - [libevent](https://libevent.org/)
 - [msgpack](https://msgpack.org/)
 - (optional) LMDB

 In this project there is no LMDB included. That means 

#### Visual Studio 2022

Here is a small guide to install VS 2022:

https://github.com/MicrosoftDocs/cpp-docs/blob/main/docs/build/vscpp-step-0-installation.md

From here we work with VS 2022.

#### libpaxos

In requirements I linked the page to libpaxos. You will land on a page with multiple versions of Paxos. 
Follow the link to [libpaxos<sup>3</sup>](https://libpaxos.sourceforge.net/paxos_projects.php#libpaxos3).
You can either use git-command in terminal to download or with clone-button when you follow the link to bitbucket.
The file will a similar name that begings with sciascid-libpaxos-< combination of number and letters >
Open folder with VS 2022.

#### WSL

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

 #### CMake

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

 libevent and msgpack.