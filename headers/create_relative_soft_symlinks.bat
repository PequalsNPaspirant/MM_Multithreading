@echo OFF
setlocal enableDelayedExpansion

cd ../src

SET CURRENT_PATH=%cd%
echo %CURRENT_PATH%

for /R %%F in (*.h) do (
	SET "FULL_PATH=%%F"
	echo "Full Path: !FULL_PATH!"
	REM echo "File Name: %%~nxF"
	REM echo "File Name without extension: %%~nF"
	REM echo "Only File Extension: %%~xF"
	
	for %%B in ("!CURRENT_PATH!") do SET "RELATIVE_PATH=!FULL_PATH:%%~B=..\src!"
	echo "Relative Path: !RELATIVE_PATH!"
	
	REM runas /user:administrator "mklink \"%%~nxF\" \"%%~F\""
	del "..\headers\%%~nxF"
	mklink "..\headers\%%~nxF" "!RELATIVE_PATH!"
)

cd ../headers/

pause