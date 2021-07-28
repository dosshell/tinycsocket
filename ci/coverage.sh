#!/bin/sh

ARG1=${1:-xml}

PROJPATH="$(dirname "$(readlink -f "$0")")"/..
mkdir $PROJPATH/build_coverage
cd $PROJPATH/build_coverage
cmake -G "Ninja" ../ -DTCS_GENERATE_COVERAGE=ON -DTCS_ENABLE_TESTS=ON
cmake --build .
./tests/tests

if [ "$ARG1" = "xml" ]; then 
    gcovr -r ../ --xml ../cobertura.xml
elif [ "$ARG1" = "html" ]; then 
    lcov --capture --directory . --output-file coverage.info
    genhtml coverage.info --output-directory ../coverage_report
else
    echo "Unknown"
fi;
