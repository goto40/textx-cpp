#!/bin/bash
set -e

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j && valgrind ./textx_unit_tests $*
