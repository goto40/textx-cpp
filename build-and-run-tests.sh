#!/bin/bash
set -e

mkdir -p build
cd build
cmake .. && make -j && ./textx_unit_tests
