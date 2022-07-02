#!/bin/bash

#Compile both travelMonitor and Monitor by executing their makefiles.
#If argument is run, executes make run with predefined arguments.
#If argument is clean or valgrind, executes make clean / make valgrind.

if (($# == 0))
then
    cd monitor
    make
    cd ../travelMonitor
    make
elif (($# == 1))
then
    if [[ $1 = "run" ]]
    then
        cd monitor
        make
        cd ../travelMonitor
        make run
    elif [[ $1 = "valgrind" ]]
    then
        cd monitor
        make
        cd ../travelMonitor
        make valgrind
    elif [[ $1 = "clean" ]]
    then
        cd monitor
        make clean
        cd ../travelMonitor
        make clean
    else
        exit 1
    fi 
else
    exit 1
fi