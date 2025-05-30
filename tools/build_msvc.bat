choco feature enable -n allowGlobalConfirmation
choco install cmake
choco install gnuwin32-m4 --version=1.4.14
call RefreshEnv.cmd
set PATH=%PATH%;C:\Program Files\CMake\bin\
mkdir build32
cd build32
cmake.exe -G "Visual Studio 17 2022" -A "win32" ../ -DTCS_ENABLE_TESTS=ON -DTCS_ENABLE_EXAMPLES=ON -DTCS_WARNINGS_AS_ERRORS=ON
cmake.exe --build . --target ALL_BUILD --config Debug
cmake.exe --build . --target ALL_BUILD --config Release
cd ..
mkdir build64
cd build64
cmake.exe -G "Visual Studio 17 2022" -A "x64" ../ -DTCS_ENABLE_TESTS=ON -DTCS_ENABLE_EXAMPLES=ON -DTCS_WARNINGS_AS_ERRORS=ON
cmake.exe --build . --target ALL_BUILD --config Debug
cmake.exe --build . --target ALL_BUILD --config Release
