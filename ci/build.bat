@echo off
setlocal enabledelayedexpansion

set SELF_DIR=%~dp0
set ROOT_DIR=%SELF_DIR%..

REM Handle Default options
if "%VS_YEAR%" == "" (
	set VS_YEAR=2019
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

if "%BUILD_DIR%" == "" (
	set BUILD_DIR=%ROOT_DIR%\build
)

if "%INSTALL_DIR%" == "" (
	set INSTALL_DIR=%ROOT_DIR%\install
)

if "%CODE_SIGN%" == "" (
	set CODE_SIGN=OFF
)

if "%PREFIX_PATH%" == "" (
	set PREFIX_PATH=""
)

if "%BUILD_SHARED%" == "" (
	set BUILD_SHARED=OFF
)

if "%BUILD_OPENSOURCE%" == "" (
	set BUILD_OPENSOURCE=ON
)

if "%DISABLE_DRIVER%" == "" (
    set DISABLE_DRIVER=ON
)

if "%DISABLE_DEMOS%" == "" (
	set DISABLE_DEMOS=OFF
)

if "%DISABLE_TOOLS%" == "" (
	set DISABLE_TOOLS=OFF
)
if "%DISABLE_PLUGINS%" == "" (
	set DISABLE_PLUGINS=OFF
)

if "%DISABLE_TESTS%" == "" (
	set DISABLE_TESTS=OFF
)

if "%INSTALL_HEADERS%" == "" (
	set INSTALL_HEADERS=ON
)

if "%INSTALL_SOURCES%" == "" (
	set INSTALL_SOURCES=ON
)

if "%INSTALL_CMAKE%" == "" (
	set INSTALL_CMAKE=ON
)

if "%INSTALL_MISC%" == "" (
	set INSTALL_MISC=ON
)

if "%QT_ENABLED%" == "" (
	set QT_ENABLED=ON
)

if "%QT_DEPLOY%" == "" (
	set QT_DEPLOY=ON
)

echo Configured Options:
echo -------------------
echo VS_YEAR: %VS_YEAR%
echo ROOT_DIR: %VS_EDITION%
echo GENERATOR: %GENERATOR%
echo BUILD_TYPE: %BUILD_TYPE%
echo PREFIX_PATH: %PREFIX_PATH%
echo BUILD_DIR: %BUILD_DIR%
echo INSTALL_DIR: %INSTALL_DIR%
echo CODE_SIGN: %CODE_SIGN%
echo BUILD_SHARED: %BUILD_SHARED%
echo BUILD_OPENSOURCE: %BUILD_OPENSOURCE%
echo DISABLE_DRIVER: %DISABLE_DRIVER%
echo DISABLE_DEMOS: %DISABLE_DEMOS%
echo DISABLE_TOOLS: %DISABLE_TOOLS%
echo DISABLE_PLUGINS: %DISABLE_PLUGINS%
echo DISABLE_TESTS: %DISABLE_TESTS%
echo INSTALL_HEADERS: %INSTALL_HEADERS%
echo INSTALL_SOURCES: %INSTALL_SOURCES%
echo INSTALL_CMAKE: %INSTALL_CMAKE%
echo INSTALL_MISC: %INSTALL_MISC%
echo QT_ENABLED: %QT_ENABLED%
echo QT_DEPLOY: %QT_DEPLOY%

REM Set up VS Environment
call "C:\Program Files (x86)\Microsoft Visual Studio\%VS_YEAR%\%VS_EDITION%\VC\Auxiliary\Build\vcvarsall.bat" x64
if not %errorlevel% == 0 (
	echo Error calling vcvarsall.bat
	exit /b 1
)

echo Removing old build/install directories
if exist %BUILD_DIR% (
    rmdir /s /q %BUILD_DIR%
)
if exist %INSTALL_DIR% (
    rmdir /s /q %INSTALL_DIR%
)

REM Generate Makefiles
cmake -S%ROOT_DIR% -B%BUILD_DIR% -G%GENERATOR% ^
-DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
-DCMAKE_PREFIX_PATH=%PREFIX_PATH% ^
-DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% ^
-DAJA_CODE_SIGN=%CODE_SIGN% ^
-DAJA_BUILD_SHARED=%BUILD_SHARED% ^
-DAJANTV2_BUILD_OPENSOURCE=%BUILD_OPENSOURCE% ^
-DAJA_DISABLE_TESTS=%DISABLE_TESTS% ^
-DAJA_DISABLE_DEMOS=%DISABLE_DEMOS% ^
-DAJA_DISABLE_TOOLS=%DISABLE_TOOLS% ^
-DAJA_DISABLE_PLUGINS=%DISABLE_PLUGINS% ^
-DAJA_INSTALL_HEADERS=%INSTALL_HEADERS% ^
-DAJA_INSTALL_SOURCES=%INSTALL_SOURCES% ^
-DAJA_INSTALL_CMAKE=%INSTALL_CMAKE% ^
-DAJA_INSTALL_MISC=%INSTALL_MISC% ^
-DAJA_QT_ENABLED=%QT_ENABLED% ^
-DAJA_QT_DEPLOY=%QT_DEPLOY% 

if not %ERRORLEVEL% == 0 (
	echo Error generating targets
	exit /b 1
)

REM Build Project
cmake --build %BUILD_DIR%
if not %errorlevel% == 0 (
	echo Error building targets
	exit /b 1
)

REM Install Project
cmake --install %BUILD_DIR% 
if not %errorlevel% == 0 (
	echo Error installing targets
	exit /b 1
)
