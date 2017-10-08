#!/bin/bash

mkdir ninja
cd ninja
cmake -G Ninja ../
cmake --build .
cd ..