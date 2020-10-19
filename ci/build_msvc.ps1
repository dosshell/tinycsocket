param(
    [switch]$dependencies = $false
)

Set-StrictMode -Version latest
$ErrorActionPreference = "Stop"

Trap {
    $Host.SetShouldExit(-1)
    Exit -1
}

if ($dependencies) {
    # install choco
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
    $env:ChocolateyInstall = Convert-Path "$((Get-Command choco).Path)\..\.."  
    Import-Module "$env:ChocolateyInstall\helpers\chocolateyProfile.psm1"

    # install dependencies
    choco feature enable -n allowGlobalConfirmation
    choco install -y visualstudio2019buildtools --force --package-parameters "--includeRecommended --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.WinXP --passive"
    choco install -y cmake
    refreshenv

    # bugfixes
    $env:Path += ";C:\Program Files\CMake\bin\"
}

mkdir "$PSScriptRoot\..\build32"
cmake.exe -S "$PSScriptRoot\..\" -B "$PSScriptRoot\..\build32" -G "Visual Studio 16 2019" -A "win32" -T v141_xp ../ -DTCS_ENABLE_TESTS=ON -DTCS_ENABLE_EXAMPLES=ON -DTCS_WARNINGS_AS_ERRORS=ON
cmake.exe --build "$PSScriptRoot\..\build32" --target ALL_BUILD --config Debug
cmake.exe --build "$PSScriptRoot\..\build32" --target ALL_BUILD --config Release

mkdir "$PSScriptRoot\..\build64"
cmake.exe -S "$PSScriptRoot\..\" -B "$PSScriptRoot\..\build64" -G "Visual Studio 16 2019" -A "x64" -T v141_xp ../ -DTCS_ENABLE_TESTS=ON -DTCS_ENABLE_EXAMPLES=ON -DTCS_WARNINGS_AS_ERRORS=ON
cmake.exe --build "$PSScriptRoot\..\build64" --target ALL_BUILD --config Debug
cmake.exe --build "$PSScriptRoot\..\build64" --target ALL_BUILD --config Release

# copy tests to common folder
mkdir  "$PSScriptRoot\..\build"
cp "$PSScriptRoot\..\build32\tests\Debug\tests.exe" "$PSScriptRoot\..\build\tests_msvc_debug_x86.exe"
cp "$PSScriptRoot\..\build32\tests\Release\tests.exe" "$PSScriptRoot\..\build\tests_msvc_release_x86.exe"
cp "$PSScriptRoot\..\build64\tests\Debug\tests.exe" "$PSScriptRoot\..\build\tests_msvc_debug_x64.exe"
cp "$PSScriptRoot\..\build64\tests\Release\tests.exe" "$PSScriptRoot\..\build\tests_msvc_release_x64.exe"
