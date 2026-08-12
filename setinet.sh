#!/bin/bash

# exit if no argument is given
if [ -z "$1" ]
  then
    echo "No argument supplied"
    exit 1
fi

# set INET_ROOT env variable in ~/.bashrc
if grep -q "INET_ROOT" ~/.bashrc
then
    sed -in "s#INET_ROOT=.*#INET_ROOT=$1#g" ~/.bashrc
else
    echo "export INET_ROOT=$1" >> ~/.bashrc
fi

