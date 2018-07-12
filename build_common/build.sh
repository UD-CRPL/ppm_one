#!/bin/bash

rm -rf CMakeCache.txt
rm -rf CMakeFiles

SOURCE=../common

cmake \
    -DCMAKE_C_COMPILER:STRING=pgcc \
    -DCMAKE_C_FLAGS:STRING="-O3 -Minfo=accel -ta=tesla:cc70" \
    -DCMAKE_CXX_COMPILER:STRING=pgc++ \
    -DCMAKE_CXX_FLAGS:STRING="-O3 -Minfo=accel -ta=tesla:cc70 -std=c++11" \
    $SOURCE
