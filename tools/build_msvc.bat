choco feature enable -n allowGlobalConfirmation
choco install cmake
call RefreshEnv.cmd
set PATH=%PATH%;C:\Program Files\CMake\bin\
mkdir build32
cd build32
cmake.exe -G "Visual Studio 16 2019" -A "win32" ../ -DTCS_ENABLE_TESTS=ON -DTCS_ENABLE_EXAMPLES=ON -DTCS_WARNINGS_AS_ERRORS=ON
cmake.exe --build . --target ALL_BUILD --config Debug
cmake.exe --build . --target ALL_BUILD --config Release
cd ..
mkdir build64
cd build64
cmake.exe -G "Visual Studio 16 2019" -A "x64" ../ -DTCS_ENABLE_TESTS=ON -DTCS_ENABLE_EXAMPLES=ON -DTCS_WARNINGS_AS_ERRORS=ON
cmake.exe --build . --target ALL_BUILD --config Debug
cmake.exe --build . --target ALL_BUILD --config Release
