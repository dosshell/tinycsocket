#/bin/bash
mkdir build
cd build
cmake -G "Unix Makefiles" ../ -DTCS_ENABLE_TESTS=ON -DTCS_ENABLE_EXAMPLES=ON -DTCS_WARNINGS_AS_ERRORS=ON
cmake --build . --config Debug
cmake --build . --config Release
