@echo off
REM ---------------------------------------------------------------------------
REM Yozora build helper for Windows.
REM
REM   scripts\build.cmd            configure + build (Release)
REM   scripts\build.cmd debug      configure + build (Debug)
REM   scripts\build.cmd test       build + run the unit tests
REM   scripts\build.cmd install    build + stage a runnable folder in dist\
REM   scripts\build.cmd package    build + build the NSIS installer
REM
REM It sets up the MSVC environment, then hands over to CMake. Qt is found
REM through CMAKE_PREFIX_PATH, which can be overridden by the caller.
REM ---------------------------------------------------------------------------

setlocal EnableDelayedExpansion

set "SOURCE_DIR=%~dp0.."
for %%I in ("%SOURCE_DIR%") do set "SOURCE_DIR=%%~fI"

if "%QT_ROOT%"=="" (
    if exist "C:\Qt\6.8.3\msvc2022_64" (
        set "QT_ROOT=C:\Qt\6.8.3\msvc2022_64"
    ) else (
        echo [yozora] Set QT_ROOT to your Qt 6 msvc2022_64 installation.
        exit /b 1
    )
)

if "%CMAKE_EXE%"=="" (
    for /f "delims=" %%I in ('where cmake 2^>nul') do (
        if not defined CMAKE_EXE set "CMAKE_EXE=%%I"
    )
)
if "%CMAKE_EXE%"=="" (
    if exist "%APPDATA%\Python\Python314\Scripts\cmake.exe" (
        set "CMAKE_EXE=%APPDATA%\Python\Python314\Scripts\cmake.exe"
    ) else (
        echo [yozora] CMake not found. Install it or set CMAKE_EXE.
        exit /b 1
    )
)

if "%NINJA_EXE%"=="" (
    if exist "%APPDATA%\Python\Python314\Scripts\ninja.exe" (
        set "NINJA_EXE=%APPDATA%\Python\Python314\Scripts\ninja.exe"
    )
)

set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=build"

if /i "%ACTION%"=="debug" (
    set "ACTION=build"
    set "BUILD_TYPE=Debug"
) else (
    set "BUILD_TYPE=Release"
)

set "BUILD_DIR=%SOURCE_DIR%\build\%BUILD_TYPE%"

REM --- MSVC environment -----------------------------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [yozora] Visual Studio Build Tools not found.
    exit /b 1
)
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * ^
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
    -property installationPath`) do set "VS_PATH=%%I"

if not defined VS_PATH (
    echo [yozora] Could not locate the MSVC toolset.
    exit /b 1
)

call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
    echo [yozora] Failed to initialise the MSVC environment.
    exit /b 1
)

set "GENERATOR=Ninja"
if not defined NINJA_EXE set "GENERATOR=Visual Studio 17 2022"

REM --- configure ------------------------------------------------------------
"%CMAKE_EXE%" -S "%SOURCE_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_PREFIX_PATH="%QT_ROOT%" ^
    -DYOZORA_BUILD_TESTS=ON
if errorlevel 1 exit /b 1

"%CMAKE_EXE%" --build "%BUILD_DIR%" --config %BUILD_TYPE%
if errorlevel 1 exit /b 1

if /i "%ACTION%"=="test" (
    "%CMAKE_EXE%" --build "%BUILD_DIR%" --target test --config %BUILD_TYPE%
    exit /b %errorlevel%
)

if /i "%ACTION%"=="install" (
    rem Stage a self-contained folder under dist\ so it can be run and tested
    rem without touching Program Files.
    "%CMAKE_EXE%" --install "%BUILD_DIR%" --config %BUILD_TYPE% ^
        --prefix "%SOURCE_DIR%\dist\%BUILD_TYPE%"
    exit /b %errorlevel%
)

if /i "%ACTION%"=="package" (
    "%CMAKE_EXE%" --build "%BUILD_DIR%" --target package --config %BUILD_TYPE%
    if errorlevel 1 exit /b 1
    echo [yozora] Installer written to %BUILD_DIR%
    exit /b 0
)

echo [yozora] Built: %BUILD_DIR%\bin\Yozora.exe
exit /b 0
