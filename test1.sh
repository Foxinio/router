#!/bin/bash

if [[ ( 1 == "$1" ) || ( 2 == "$1" ) || ( 3 == "$1" ) || ( 4 == "$1" ) ]]; then
  [[ ! -e ./test/test1/configured ]] && ./test/test1/configure.sh $1
  ./router < "./test/test1/config$1"
fi
