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

echo "Copying takeme (CDROM filesystem contents)"
xcopy "%SRC_PATH%\takeme" "%DST_PATH%\takeme" /Q /E /I /C /-Y
IF EXIST "%DST_PATH%\data" (
   echo "Copying data/* into takeme/"
   xcopy "%DST_PATH%\data\*" "%DST_PATH%\takeme" /Q /E /I /C /-Y
)
IF NOT EXIST "%DST_PATH%\src" (
   echo "Copying src"
   xcopy "%SRC_PATH%\src" "%DST_PATH%\src" /Q /E /I /C /-Y
)

echo "Copying build files"
xcopy /Q /Y "%SRC_PATH%\activate-env.bat" "%DST_PATH%"
xcopy /Q /Y "%SRC_PATH%\makefile" "%DST_PATH%"
xcopy /Q /Y "%SRC_PATH%\make.bat" "%DST_PATH%"
xcopy /Q /Y "%SRC_PATH%\make-run.bat" "%DST_PATH%"
xcopy /Q /Y "%SRC_PATH%\make-clean.bat" "%DST_PATH%"

if [%arg0:~2,1%]==[:] pause
