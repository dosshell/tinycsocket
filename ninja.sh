#!/bin/bash

mkdir ninja
cd ninja
cmake -G Ninja  -DCMAKE_BUILD_TYPE=debug ../
cmake --build .
cd ..
