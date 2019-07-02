Elastos Hive Native SDK  
==========================

|Linux & mac|Windows|
|:-:|:-:|
|[![Build Status](https://travis-ci.org/elastos/Elastos.NET.Hive.Native.SDK.svg)](https://travis-ci.org/elastos/Elastos.NET.Hive.Native.SDK)|[![Build status](https://ci.appveyor.com/api/projects/status/at2povuwuw9soaxf?svg=true)](https://ci.appveyor.com/project/Elastos/elastos-net-hive-native-sdk)|
  
## Introduction  
  
Elastos Hive Native SDK provides a set of APIs to enable SDK users to perform file operations on various storage backends(e.g., OneDrive, IPFS, DropBox, WebDAV, etc..., only OneDrive and IPFS are implemented currently) in a unified manner.  
  
## Table of Contents  
  
- [Elastos Hive Native SDK](#elastos-hive-native-sdk)  
   - [Introduction](#introduction)  
   - [Table of Contents](#table-of-contents)  
   - [Usage](#usage)  
   - [Build on Ubuntu / Debian / Linux Host](#build-on-ubuntu-debian-linux-host)  
      - [1. Install Pre-Requirements](#1-install-pre-requirements)  
      - [2. Build to run on host (Ubuntu / Debian / Linux)](#2-build-to-run-on-host-ubuntu-debian-linux)  
      - [3. Run HiveDemo](#3-run-HiveDemo)  
   - [Build on MacOS Host](#build-on-macos-host)  
      - [1. Install Pre-Requirements](#1-install-pre-requirements)  
      - [2. Build to run on host](#2-build-to-run-on-host)  
      - [3. Run HiveDemo](#3-run-HiveDemo)  
   - [Build on Windows Host](#build-on-windows-host)  
      - [1. Brief introduction](#1-brief-introduction)  
      - [2. Set up Environment](#2-set-up-environment)  
      - [3. Build to run on host](#3-build-to-run-on-host)  
      - [4. Run HiveDemo](#4-run-HiveDemo)  
   - [Build API Documentation](#build-api-documentation)  
   - [Build on Ubuntu / Debian / Linux Host](#build-on-ubuntu-debian-linux-host)  
      - [1. Install Pre-Requirements](#1-install-pre-requirements)  
      - [2. Build](#2-build)  
      - [3. View](#3-view)  
   - [Contribution](#contribution)  
   - [Acknowledgments](#acknowledgments)  
   - [License](#license)  
  
  
## Usage  
  
**CMake** is used to build, test and package the Elastos Hive project in an operating system as well as compiler-independent manner.  
  
Certain knowledge of CMake is required.  
  
The compilation of source works on **macOS**, **Linux** (Ubuntu, Debian etc.) and **Windows**.  
  
## Build on Ubuntu / Debian / Linux Host  
  
#### 1. Install Pre-Requirements  
  
To generate Makefiles by using **configure** or **cmake** and manage dependencies of the Hive project, certain packages must be installed on the host before compilation.  
  
Run the following commands to install the prerequisite utilities:  
  
```shell  
$ sudo apt-get update  
$ sudo apt-get install -f build-essential autoconf automake autopoint libtool flex bison libncurses5-dev cmake  
```  
  
Download this repository using Git:  
```shell  
$ git clone https://github.com/elastos/Elastos.NET.Hive.Native.SDK  
```  
  
#### 2. Build to run on host (Ubuntu / Debian / Linux)  
  
To compile the project from source code for the target to run on Ubuntu / Debian / Linux, carry out the following steps:  
  
Open a new terminal window.  
  
Navigate to the previously downloaded folder that contains the source code of the Hive project.  
  
```shell  
$ cd YOUR-PATH/Elastos.NET.Hive.Native.SDK  
```  
  
Enter the 'build' folder.  
  
```shell  
$ cd build  
```  
  
Create a new folder with the target platform name, then change directory.  
  
```shell  
$ mkdir linux  
$ cd linux  
```  
  
Generate the Makefile in the current directory:  
  
*Note: Please see custom options below.*  
  
```shell  
$ cmake ../..  
```  
  
***  
Optional (Generate the Makefile): To be able to build a distribution with a specific build type **Debug/Release**, as well as with customized install location of distributions, run the following commands:  
  
```shell  
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=YOUR-INSTALL-PATH ../..  
```  
***  
  
Build the program:  
  
*Note: If "make" fails due to missing permissions, use "sudo make" instead.*  
  
```shell  
$ make  
```  
  
Install the program:  
  
*Note: If "make install" fails due to missing permissions, use "sudo make install" instead.*  
  
```shell  
$ make install  
```  
  
Create distribution package:  
  
*Note: If "make dist" fails due to missing permissions, use "sudo make dist" instead.*  
  
```shell  
$ make dist  
```  
  
#### 3. Run HiveDemo
  
HiveDemo is a fully functional, lightweight shell program that processes commands and returns the output to the terminal. Each Hive API has its corresponding command. This program is intended to familiarize users with the use of Hive APIs.
  
To run HiveDemo, first extract the distribution package created previously and enter the extracted folder. Then, change directory to the 'bin' folder.  
  
```shell  
$ cd YOUR-DISTRIBUTION-PACKAGE-PATH/bin  
```  
  
Run HiveDemo:  
  
```shell  
$ ./hivedemo --config=YOUR-DISTRIBUTION-PACKAGE-PATH/etc/hive/hivedemo.conf
```  
  
Available commands in the shell can be listed by using the command **help**. Specific command usage descriptions can be displayed by using **help [Command]** where [Command] must be replaced with the specific command name.

## Build on MacOS Host  

#### 1. Install Pre-Requirements  
  
packages must be installed on the host before compilation.  
  
The following packages related to **configure** and **cmake** must be installed on the host before compilation either by installation through the package manager **homebrew** or by building from source:  
  
Note: Homebrew can be downloaded from https://brew.sh/ .  
  
  
Install packages with Homebrew:  
  
```shell  
$ brew install autoconf automake libtool shtool pkg-config gettext cmake  
```  
  
Please note that **homebrew** has an issue with linking **gettext**. If you have an issue with the execution of **autopoint**, fix it by running:  
  
```shell  
$ brew link --force gettext  
```  
  
Download this repository using Git:  
  
```shell  
$ git clone https://github.com/elastos/Elastos.NET.Hive.Native.SDK  
```  
  
#### 2. Build to run on host  
  
To compile the project from source code for the target to run on MacOS, carry out the following steps:  
  
Open a new terminal window.  
  
Navigate to the previously downloaded folder that contains the source code of the Carrier project.  
  
```shell  
$ cd YOUR-PATH/Elastos.NET.Hive.Native.SDK  
```  
  
Enter the 'build' folder.  
  
```shell  
$ cd build  
```  
  
Create a new folder with the target platform name, then change directory.  
  
```shell  
$ mkdir macos  
$ cd macos  
```  
  
Generate the Makefile in the current directory:  
  
*Note: Please see custom options below.*  
  
```shell  
$ cmake ../..  
```  
  
***  
Optional (Generate the Makefile): To be able to build a distribution with a specific build type **Debug/Release**, as well as with customized install location of distributions, run the following commands:  
```shell  
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=YOUR-INSTALL-PATH ../..  
```  
***  
  
Build the program:  
  
*Note: If "make" fails due to missing permissions, use "sudo make" instead.*  
  
```shell  
$ make  
```  
  
Install the program:  
  
*Note: If "make install" fails due to missing permissions, use "sudo make install" instead.*  
  
```shell  
$ make install  
```  
  
Create distribution package:  
  
*Note: If "make dist" fails due to missing permissions, use "sudo make dist" instead.*  
  
```shell  
$ make dist  
```  
  
#### 3. Run HiveDemo
  
HiveDemo is a fully functional, lightweight shell program that processes commands and returns the output to the terminal. Each Hive API has its corresponding command. This program is intended to familiarize users with the use of Hive APIs.
  
To run HiveDemo, first extract the distribution package created previously and enter the extracted folder. Then, change directory to the 'bin' folder.  
  
```shell  
$ cd YOUR-DISTRIBUTION-PACKAGE-PATH/bin  
```  
  
Run HiveDemo:  
  
```shell  
$ ./hivedemo --config=YOUR-DISTRIBUTION-PACKAGE-PATH/etc/hive/hivedemo.conf
```  
  
Available commands in the shell can be listed by using the command **help**. Specific command usage descriptions can be displayed by using **help [Command]** where [Command] must be replaced with the specific command name.   

## Build on Windows Host  
  
#### 1. Brief introduction  
  
With CMake, Elastos Hive can be cross-compiled to run only on Windows as target platform, while compilation is carried out on a Windows host. Both 32-bit and 64-bit target versions are supported.  
  
#### 2. Set up Environment  
  
**Prerequisites**:  
- Visual Studio IDE is required. The Community version can be downloaded at https://visualstudio.microsoft.com/downloads/ for free.  
- Download and install "Visual Studio Command Prompt (devCmd)" from https://marketplace.visualstudio.com/items?itemName=ShemeerNS.VisualStudioCommandPromptdevCmd .  
- Install 'Desktop development with C++' Workload  
  
Start the program 'Visual Studio Installer'.  
***  
Alternative:  
Start Visual Studio IDE.  
In the menu, go to "Tools >> Get Tools and Features", it will open the Visual Studio Installer.  
***  
  
Make sure 'Desktop development with C++' Workload is installed.  
  
On the right side, make sure in the 'Installation details' all of the following are installed:  
  
- "Windows 8.1 SDK and UCRT SDK" <- might have to be selected additionally  
- "Windows 10 SDK (10.0.17134.0)" <- might have to be selected additionally  
- "VC++ 2017 version 15.9 ... tools"  
- "C++ Profiling tools"  
- "Visual C++ tools for CMake"  
- "Visual C++ ATL for x86 and x64"  
  
Additional tools are optional, some additional ones are installed by default with the Workload.  
  
After modifications, restarting of Visual Studio might be required.  
  
#### 3. Build to run on host  
  
To compile the project from source code for the target to run on Windows, carry out the following steps:  
  
In Visual Studio, open Visual Studio Command Prompt from the menu "Tools >> Visual Studio Command Prompt". It will open a new terminal window.  
  
***  
Note: To build for a 32-bit target , select `x86 Native Tools Command Console` to run building commands, otherwise, select `x64 Native Tools Command Console` for a 64-bit target.  
***  
  
Navigate to the previously downloaded folder that contains the source code of the Hive project.  
  
```shell  
$ cd YOUR-PATH/Elastos.NET.Hive.Native.SDK  
```  
  
Enter the 'build' folder.  
  
```shell  
$ cd build  
```  
  
Create a new folder with the target platform name, then change directory.  
  
```shell  
$ mkdir win  
$ cd win  
```  
  
Generate the Makefile in the current directory:  
  
```shell  
$ cmake -G "NMake Makefiles" -DCMAKE_INSTALL_PREFIX=outputs ..\..  
```  
  
Build the program:  
  
```shell  
$ nmake  
```  
  
Install the program:  
```shell  
$ nmake install  
```  
  
Create distribution package:  
  
```shell  
$ nmake dist  
```  
  
#### 4. Run HiveDemo
  

HiveDemo is a fully functional, lightweight shell program that processes commands and returns the output to the terminal. Each Hive API has its corresponding command. This program is intended to familiarize users with the use of Hive APIs.
  
To run HiveDemo, first extract the distribution package created previously and enter the extracted folder. Then, change directory to the 'bin' folder.  

```shell  
$ cd YOUR-DISTRIBUTION-PACKAGE-PATH\bin  
```  
  
Run HiveDemo:  
  
*Make sure to replace 'YOUR-DISTRIBUTION-PACKAGE-PATH'.*  
  
```shell  
$ hivedemo.exe --config=YOUR-DISTRIBUTION-PACKAGE-PATH\etc\hive\hivedemo.conf  
```  
  
Available commands in the shell can be listed by using the command **help**. Specific command usage descriptions can be displayed by using **help [Command]** where [Command] must be replaced with the specific command name. 

## Build API Documentation  
  
Currently, the API documentation can only be built on **Linux** hosts. MacOS has a bug issue with python, which would cause build process failure.  
  
## Build on Ubuntu / Debian / Linux Host  
  
#### 1. Install Pre-Requirements  
  
```shell  
$ sudo apt-get update  
$ sudo apt-get install doxygen python-sphinx graphviz  
$ curl -L -o /tmp/get-pip.py https://bootstrap.pypa.io/get-pip.py  
$ sudo python /tmp/get-pip.py  
$ sudo pip install breathe  
```  
  
#### 2. Build  
  
Change to the directory where the build for any target has been previously executed. For example, if the target was Linux, the folder structure would be similar to:  
  
```shell  
cd /YOUR-PATH/Elastos.NET.Hive.Native.SDK/build/linux  
```  
  
Run the following command:  
  
*Note: If "make" fails due to missing permissions, use "sudo make" instead.*  
  
```shell  
$ cmake -DENABLE_DOCS=ON ../..  
$ make  
```  
  
#### 3. View  
  
The generated API documentation will be created in the new **/docs** directory on the same directory level.  
  
Change to the docs folder:  
  
```shell  
cd docs/html  
```  
Open the index.html file in a browser from the terminal:  
  
```shell  
xdg-open index.html  
```  
  
## Contribution  
  
We welcome contributions to the Elastos Hive Project.  
  
## Acknowledgments  
  
A sincere thank you to all teams and projects that we rely on directly or indirectly.  
  
## License  
  
This project is licensed under the terms of the [MIT license](https://github.com/elastos/Elastos.NET.Carrier.Native.SDK/blob/master/LICENSE).
