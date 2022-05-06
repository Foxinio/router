#!/bin/bash

if [[ ( 1 == "$1" ) || ( 2 == "$1" ) || ( 3 == "$1" ) || ( 4 == "$1" ) ]]; then
  [[ ! -e ./test/test2/configured ]] && ./test/test2/configure.sh $1
  touch ./test/test2/configured
  ./router < "./test/test2/config$1" 2> debug.out
fi
