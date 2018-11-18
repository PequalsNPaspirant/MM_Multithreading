@echo OFF

cd ../src

for /R %%F in (*.h) do (
	echo "full path and name: %%F"
	echo "name: %%~nxF"
	echo "name without extension: %%~nF"
	echo "only extension: %%~xF"
	REM runas /user:administrator "mklink \"%%~nxF\" \"%%~F\""
	mklink "../headers/%%~nxF" "%%~F"
)

cd ../headers/

pause