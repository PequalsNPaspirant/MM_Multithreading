@ECHO OFF
SETLOCAL

set BUILD_DIR="%CD%\build_vs_2022"
echo "BUILD_DIR: %BUILD_DIR%"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

set CLEANBUILD=%1

IF /I "%CLEANBUILD%"=="clean" (
	echo "Cleaning build directory %BUILD_DIR%. Press any key to continue..."
	pause
	del /S /Q "%BUILD_DIR%\*.*"
)

cmake -H. -Bbuild_vs_2022 -G "Visual Studio 17 2022" -A x64

cmake --build %BUILD_DIR% --config RelWithDebInfo

pause
