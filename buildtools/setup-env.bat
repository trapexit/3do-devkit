@echo off

set arg0=%0

if defined TDO_DEVKIT_PATH (
  echo Environment is already setup. TDO_DEVKIT_PATH is set to %TDO_DEVKIT_PATH%
  if [%arg0:~2,1%]==[:] pause
  exit /b
)

set "TDO_DEVKIT_PATH=%~dp0"
set "TDO_DEVKIT_PATH=%TDO_DEVKIT_PATH:~0,-1%"
for %%i in ("%TDO_DEVKIT_PATH%") do set "TDO_DEVKIT_PATH=%%~dpi"
set "TDO_DEVKIT_PATH=%TDO_DEVKIT_PATH:~0,-1%"
setx TDO_DEVKIT_PATH "%TDO_DEVKIT_PATH%"
setx PATH "%TDO_DEVKIT_PATH%\bin\compiler\win;%TDO_DEVKIT_PATH%\bin\tools\win;%PATH%"

if [%arg0:~2,1%]==[:] pause
