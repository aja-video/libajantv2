@echo off
setlocal enabledelayedexpansion

set SELF_DIR=%~dp0
set ROOT_DIR=%SELF_DIR%..

REM Handle Default options
if "%VS_YEAR%" == "" (
	set VS_YEAR=2017
)

if "%VS_EDITION%" == "" (
	set VS_EDITION=Professional
)

if "%GENERATOR%" == "" (
	set GENERATOR=Ninja
)

if "%BUILD_TYPE%" == "" (
	set BUILD_TYPE=Release
)

if "%PREFIX_PATH%" == "" (
	set PREFIX_PATH=""
)

if "%BUILD_DIR%" == "" (
	set BUILD_DIR=%ROOT_DIR%\build
)

if "%INSTALL_DIR%" == "" (
	set INSTALL_DIR=%ROOT_DIR%\install
)

echo Configured Options:
echo -------------------
echo VS_YEAR:     %VS_YEAR%
echo ROOT_DIR:    %VS_EDITION%
echo GENERATOR:   %GENERATOR%
echo BUILD_TYPE:  %BUILD_TYPE%
echo PREFIX_PATH: %PREFIX_PATH%
echo BUILD_DIR:   %BUILD_DIR%
echo INSTALL_DIR: %INSTALL_DIR%
echo -------------------

REM Set up VS Environment
call "C:\Program Files (x86)\Microsoft Visual Studio\%VS_YEAR%\%VS_EDITION%\VC\Auxiliary\Build\vcvarsall.bat" x64
if not %errorlevel% == 0 (
	echo Error calling vcvarsall.bat
	exit 1
)

REM Generate Makefiles
cmake -S%ROOT_DIR% -B%BUILD_DIR% -G%GENERATOR% -DCMAKE_PREFIX_PATH=%PREFIX_PATH% -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if not %errorlevel% == 0 (
	echo Error generating Makefiles
	exit 1
)

REM Build Project
cmake --build %BUILD_DIR%
if not %errorlevel% == 0 (
	echo Error building project
	exit 1
)

REM Install Project
cmake --install %BUILD_DIR% 
if not %errorlevel% == 0 (
	echo Error installing project
	exit 1
)
