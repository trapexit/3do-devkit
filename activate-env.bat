@echo off

if "%1"=="deactivate" goto :deactivate-env
if "%1"=="d" goto :deactivate-env

if defined TDO_DEVKIT_PATH (
    goto :add_path
)

if exist ".devkit-path" (
    for /f "delims=" %%a in (.devkit-path) do set "TDO_DEVKIT_PATH=%%a"
    goto :add_path
)

set "TDO_DEVKIT_PATH=%~dp0"
set "TDO_DEVKIT_PATH=%TDO_DEVKIT_PATH:~0,-1%"

:add_path
set "PATH=%PATH%;%TDO_DEVKIT_PATH%\bin\compiler\win"
set "PATH=%PATH%;%TDO_DEVKIT_PATH%\bin\tools\win"
set "PATH=%PATH%;%TDO_DEVKIT_PATH%\bin\buildtools\win"

where armcc >nul 2>&1
if %errorlevel% equ 0 (
    echo 3DO development environment activated. Run 'activate-env.bat deactivate' to deactivate.
    goto :eof
) else (
    echo Error: Activation failed. armcc not found in PATH.
    if exist ".devkit-path" (
        echo Note: .devkit-path contains: "%TDO_DEVKIT_PATH%"
        echo If this path is wrong, update .devkit-path.
    )
    goto :eof
)

:deactivate-env
set "PATH=%PATH:;%TDO_DEVKIT_PATH%\bin\compiler\win=%"
set "PATH=%PATH:;%TDO_DEVKIT_PATH%\bin\tools\win=%"
set "PATH=%PATH:;%TDO_DEVKIT_PATH%\bin\buildtools\win=%"
set "TDO_DEVKIT_PATH="
echo 3DO development environment deactivated.
goto :eof
