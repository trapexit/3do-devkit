@echo off

set arg0=%0

if defined TDO_DEVKIT_PATH (
  echo Environment is already setup.
  echo TDO_DEVKIT_PATH is set to %TDO_DEVKIT_PATH%
  echo Go into System Properties to remove existing setup and run again if needed.
  if [%arg0:~2,1%]==[:] pause
  start rundll32.exe sysdm.cpl,EditEnvironmentVariables
  exit /b
)

set "TDO_DEVKIT_PATH=%~dp0"
set "TDO_DEVKIT_PATH=%TDO_DEVKIT_PATH:~0,-1%"
for %%i in ("%TDO_DEVKIT_PATH%") do set "TDO_DEVKIT_PATH=%%~dpi"
set "TDO_DEVKIT_PATH=%TDO_DEVKIT_PATH:~0,-1%"
for %%i in ("%TDO_DEVKIT_PATH%") do set "TDO_DEVKIT_PATH=%%~dpi"
set "TDO_DEVKIT_PATH=%TDO_DEVKIT_PATH:~0,-1%"
for %%i in ("%TDO_DEVKIT_PATH%") do set "TDO_DEVKIT_PATH=%%~dpi"
set "TDO_DEVKIT_PATH=%TDO_DEVKIT_PATH:~0,-1%"
setx TDO_DEVKIT_PATH "%TDO_DEVKIT_PATH%"


set "PATH=%TDO_DEVKIT_PATH%\bin\compiler\win;%PATH%"
set "PATH=%TDO_DEVKIT_PATH%\bin\tools\win;%PATH%"
set "PATH=%TDO_DEVKIT_PATH%\bin\buildtools\win;%PATH%"
setx PATH "%PATH%"

echo TDO_DEVKIT_PATH set to %TDO_DEVKIT_PATH%
echo PATH set to %PATH%

if [%arg0:~2,1%]==[:] pause
