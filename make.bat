@echo off
setlocal DisableDelayedExpansion

if exist "%~dp0activate-env.bat" (
    call "%~dp0activate-env.bat"
) else if defined TDO_DEVKIT_PATH (
    call "%TDO_DEVKIT_PATH%\activate-env.bat"
) else (
    echo Error: Cannot locate the DevKit. Set TDO_DEVKIT_PATH to the installed DevKit directory.
    exit /b 1
)
if errorlevel 1 (
    echo Error: DevKit activation failed.
    exit /b 1
)
where armcc >nul 2>&1
if errorlevel 1 (
    echo Error: Activation failed. armcc is not on PATH.
    exit /b 1
)

make.exe %*
set "status=%errorlevel%"
set "arg0=%0"
if [%arg0:~2,1%]==[:] pause
endlocal & exit /b %status%
