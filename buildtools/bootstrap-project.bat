@echo off

set arg0=%0
set SRC_PATH=%TDO_DEVKIT_PATH%
set DST_PATH=%cd%

if "%SRC_PATH:~-1%"=="\" set SRC_PATH=%SRC_PATH:~0,-1%
if "%DST_PATH:~-1%"=="\" set DST_PATH=%DST_PATH:~0,-1%

if /I "%DST_PATH%"=="%SRC_PATH%" (
    echo Error: Target path is the same as the source path. Aborting.
    if [%arg0:~2,1%]==[:] pause
    exit /b 1
)

rem Check if the 'takeme' folder already exists in the target path
if exist "%DST_PATH%\takeme\" (
    echo Error: 'takeme' folder already exists in the target path. Aborting.
    if [%arg0:~2,1%]==[:] pause
    exit /b 1
)

xcopy "%SRC_PATH%\takeme" "%DST_PATH%\takeme" /E /I /C /-Y
xcopy "%SRC_PATH%\src" "%DST_PATH%\src" /E /I /C /-Y
xcopy /Y "%SRC_PATH%\activate-env.bat" "%DST_PATH%"
xcopy /Y "%SRC_PATH%\makefile" "%DST_PATH%"
xcopy /Y "%SRC_PATH%\make.bat" "%DST_PATH%"
xcopy /Y "%SRC_PATH%\make-run.bat" "%DST_PATH%"
xcopy /Y "%SRC_PATH%\make-clean.bat" "%DST_PATH%"

if [%arg0:~2,1%]==[:] pause
