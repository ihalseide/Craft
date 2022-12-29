@echo off
del craft.exe
rem cmake -G "MinGW Makefiles" || exit /B 0
rem mingw32-make -B > LastBuildLog.txt 2>&1 && exit /B
mingw32-make > LastBuildLog.txt 2>&1 && exit /B
type LastBuildLog.txt
echo FAILED
exit /B 0
