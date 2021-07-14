# Installation Guide

## Prerequisites

To install this application you will need these packages:
* cmake
* build-essential

## Configuring your system

To use this library in your system you need to modify the following parameters
* In ```src/rs232.c``` modify at line 53 ```"/dev/ttymxc1"``` with the serial device connected to your nxp boad.
* In ```samples/cpp/authenticate.cc``` modify at line 13 the admin userid with yours, at line 165 ```"/dev/ttyACM0"``` with the serial connected to your camera and at line 172 ```Gpio(1, 6)``` with your gpio definition.

## Installing

To install the library with the voice activation example follow this instruction

1. Create a build folder
```bash
mkdir build
cd build
```
2. Launch cmake to generate makefiles
```bash
cmake ..
```
3. Build the library and the examples
```bash
make -j
```
## Usage

To use this sample, first you need to enroll the admin user running either ```build/bin/enroll-cpp-sample``` or ```build/bin/rsid-cli```.

To run the example run
```bash
./build/bin/authenticate-cpp-sample
```
