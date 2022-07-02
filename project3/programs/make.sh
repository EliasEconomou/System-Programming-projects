#!/bin/bash

#Compile both travelMonitor and Monitor by executing their makefiles.
#If argument is run, executes make run with predefined arguments.
#If argument is clean or valgrind, executes make clean / make valgrind.

if (($# == 0))
then
    cd monitorServer
    make
    cd ../travelMonitorClient
    make
elif (($# == 1))
then
    if [[ $1 = "run" ]]
    then
        cd monitorServer
        make
        cd ../travelMonitorClient
        make run
    elif [[ $1 = "valgrind" ]]
    then
        cd monitorServer
        make
        cd ../travelMonitorClient
        make valgrind
    elif [[ $1 = "clean" ]]
    then
        cd monitorServer
        make clean
        cd ../travelMonitorClient
        make clean
    else
        exit 1
    fi 
else
    exit 1
fi