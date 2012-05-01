#!/bin/bash

PLATFORM=`uname`
SUPPRESSIONS=""

if [ "$PLATFORM" = "Linux" ]; then
  SUPPRESSIONS="--suppressions=third_party/linux-suppressions.txt"
fi

if [ "$PLATFORM" = "Darwin" ]; then
  SUPPRESSIONS="--suppressions=third_party/osx-suppressions.txt"
fi

VALGRIND="valgrind --leak-check=full --show-reachable=yes $SUPPRESSIONS --track-fds=yes"
VALGRIND_TO_LOG="$VALGRIND --log-file=valgrind.log.tmp"

run_valgrind() {
  echo "$@" >> valgrind.log
  $VALGRIND_TO_LOG $@
  cat valgrind.log.tmp >> valgrind.log
  echo >> valgrind.log
  echo >> valgrind.log
  echo >> valgrind.log
}

if [ -z "$*" ]; then
  echo -n > valgrind.log
  run_valgrind build/src/base/base_tests --valgrind --gtest_filter=-LoggingTest.*
  rm valgrind.log.tmp
else
  $VALGRIND $*
fi
