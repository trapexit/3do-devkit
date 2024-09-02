@echo off

call activate-env.bat
make.exe %*
set arg0=%0
if [%arg0:~2,1%]==[:] pause
