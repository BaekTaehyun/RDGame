@echo off
setlocal EnableDelayedExpansion
REM SimpleMMO Server Build Script
REM "x64 Native Tools Command Prompt for VS 2022" 에서 실행하세요!

set SODIUM_INCLUDE=..\Plugins\GsNetworking\Source\ThirdParty\libsodium\include
set SODIUM_LIB=..\Plugins\GsNetworking\Source\ThirdParty\libsodium\lib\windows-x64\libsodium.lib

echo ============================================
echo Building SimpleMMO Server (x64) with encryption...
echo ============================================
echo.


REM Check if cl.exe is available
where cl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] cl.exe not found. Attempting to set up VS 2022 x64 environment...
    
    set "VS_PATH="
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )

    if defined VS_PATH (
        echo [INFO] Found VS 2022 at: "!VS_PATH!"
        call "!VS_PATH!"
    ) else (
        echo [ERROR] Could not find Visual Studio 2022 installation automatically.
        echo Please manually open "x64 Native Tools Command Prompt for VS 2022" and run this script.
        pause
        exit /b 1
    )
)

echo [OK] Environment check passed.



REM Stop existing server if running
taskkill /F /IM SimpleMMO_Server.exe >nul 2>nul

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
