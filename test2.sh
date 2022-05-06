#!/bin/bash

if [[ ( 1 == "$1" ) || ( 2 == "$1" ) || ( 3 == "$1" ) || ( 4 == "$1" ) ]]; then
  ./test/test2/configure.sh $1
  ./router < "./test/test2/config$1"
fi
