@echo off

if defined TDO_DEVKIT_PATH (exit /b)

set "TDO_DEVKIT_PATH=%~dp0"
set "TDO_DEVKIT_PATH=%TDO_DEVKIT_PATH:~0,-1%"
set "PATH=%TDO_DEVKIT_PATH%\bin\compiler\win;%TDO_DEVKIT_PATH%\bin\tools\win;%PATH%"
