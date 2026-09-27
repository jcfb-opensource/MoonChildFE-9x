@echo off
REM Builds MoonChildFE (Win98 Edition) using the MSYS2 mingw32 (i686-w64-mingw32) toolchain.
REM Usage: BuildGameWindows.bat [Debug|Release]
setlocal EnableDelayedExpansion

set "CONFIG=Debug"
set "PRESET=win98-debug"
set "BUILD_PRESET=build-win98-debug"

for %%A in (%*) do (
    if /I "%%A"=="Release" (
        set "CONFIG=Release"
        set "PRESET=win98-release"
        set "BUILD_PRESET=build-win98-release"
    ) else if /I "%%A"=="Debug" (
        set "CONFIG=Debug"
        set "PRESET=win98-debug"
        set "BUILD_PRESET=build-win98-debug"
    )
)

for /f %%a in ('echo prompt $E ^| cmd') do (
    set "ESC=%%a"
)
set "R_ERR=%ESC%[41m"
set "R_OK=%ESC%[32m"
set "R_LOG=%ESC%[35m"
set "R_0=%ESC%[0m"

if not defined MOONCHILD_MINGW32_ROOT (
    set "MOONCHILD_MINGW32_ROOT=C:\msys64\mingw32"
)

if not exist "%MOONCHILD_MINGW32_ROOT%\bin\gcc.exe" (
    echo %R_ERR%mingw32 gcc.exe not found under "%MOONCHILD_MINGW32_ROOT%"! Install MSYS2 and the mingw32 package group ^(pacman -S --needed base-devel mingw-w64-i686-toolchain^), or set MOONCHILD_MINGW32_ROOT.%R_0%
    exit /b 1
)

set "PATH=%MOONCHILD_MINGW32_ROOT%\bin;%PATH%"

cd /d "%~dp0.."

echo %R_LOG%Configuring the game... ^(Win98 / !CONFIG!^)%R_0%
cmake --preset "!PRESET!"
if %ERRORLEVEL% neq 0 (
    exit /b %ERRORLEVEL%
)

echo %R_LOG%Building the game...%R_0%
cmake --build --preset "!BUILD_PRESET!" --target "MoonChildFE" --parallel "%NUMBER_OF_PROCESSORS%"
if %ERRORLEVEL% neq 0 (
    exit /b %ERRORLEVEL%
)

echo %R_OK%Build complete! ^(Win98 / %CONFIG%^).%R_0%
