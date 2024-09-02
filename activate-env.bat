@echo off

if not defined TDO_DEVKIT_PATH (
  set TDO_DEVKIT_PATH=%~dp0
  set PATH=%TDO_DEVKIT_PATH%bin\compiler\win;%TDO_DEVKIT_PATH%bin\tools\win;%PATH%
)
