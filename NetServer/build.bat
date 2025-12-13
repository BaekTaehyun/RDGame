@echo off
REM SimpleMMO Server Build Script
REM "x64 Native Tools Command Prompt for VS 2022" 에서 실행하세요!

set SODIUM_INCLUDE=..\Plugins\GsNetworking\Source\ThirdParty\libsodium\include
set SODIUM_LIB=..\Plugins\GsNetworking\Source\ThirdParty\libsodium\lib\windows-x64\libsodium.lib

echo ============================================
echo Building SimpleMMO Server (x64) with encryption...
echo ============================================
echo.

REM Check if running in x64 environment
if "%VSCMD_ARG_TGT_ARCH%"=="x64" (
    echo [OK] x64 environment detected
) else (
    echo [WARNING] This may not be x64 environment!
    echo Please use "x64 Native Tools Command Prompt for VS 2022"
    echo.
)

REM SODIUM_STATIC is required for static linking
cl /std:c++17 /EHsc /utf-8 /DSODIUM_STATIC /I%SODIUM_INCLUDE% main.cpp %SODIUM_LIB% ws2_32.lib advapi32.lib /Fe:SimpleMMO_Server.exe

if %ERRORLEVEL% == 0 (
    echo.
    echo ============================================
    echo Build successful! Run: SimpleMMO_Server.exe
    echo ============================================
) else (
    echo.
    echo ============================================
    echo Build failed with error code %ERRORLEVEL%
    echo.
    echo Make sure you are using:
    echo "x64 Native Tools Command Prompt for VS 2022"
    echo ============================================
)

pause
