#!/bin/bash
set -e

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j && ./textx_unit_tests --benchmark-samples 10 $*
