@echo off

setlocal
set "arg0=%0"
if not defined TDO_DEVKIT_PATH (
    echo Error: Source the DevKit environment before bootstrapping ^(TDO_DEVKIT_PATH is not set^).
    exit /b 1
)
where armcc >nul 2>&1
if errorlevel 1 (
    echo Error: Source the DevKit environment before bootstrapping ^(armcc is not on PATH^).
    exit /b 1
)

set "SRC_PATH=%TDO_DEVKIT_PATH%"
set "DST_PATH=%cd%"

if "%SRC_PATH:~-1%"=="\" set "SRC_PATH=%SRC_PATH:~0,-1%"
if "%DST_PATH:~-1%"=="\" set "DST_PATH=%DST_PATH:~0,-1%"

if /I "%DST_PATH%"=="%SRC_PATH%" (
    echo Error: Target path is the same as the source path. Aborting.
    if [%arg0:~2,1%]==[:] pause
    exit /b 1
)

if exist "%DST_PATH%\takeme\" (
    echo "'takeme' directory exists. Skipping."
) ELSE (
    echo "Copying takeme (core CDROM filesystem contents)"
    mkdir "%DST_PATH%\takeme"
    if errorlevel 1 goto failed
    copy /Y "%SRC_PATH%\takeme\AppStartup" "%DST_PATH%\takeme\AppStartup"
    if errorlevel 1 goto failed
    copy /Y "%SRC_PATH%\takeme\BannerScreen" "%DST_PATH%\takeme\BannerScreen"
    if errorlevel 1 goto failed
    xcopy "%SRC_PATH%\takeme\System" "%DST_PATH%\takeme\System" /Q /E /I /-Y
    if errorlevel 1 goto failed
    IF EXIST "%DST_PATH%\data" (
       echo "Copying data/* into takeme/"
       xcopy "%DST_PATH%\data\*" "%DST_PATH%\takeme" /Q /E /I /-Y
       if errorlevel 2 goto failed
    )
)
IF NOT EXIST "%DST_PATH%\src" (
   echo "Copying starter src/main.c"
   mkdir "%DST_PATH%\src"
   if errorlevel 1 goto failed
   copy /Y "%SRC_PATH%\src\helloworld\main.c" "%DST_PATH%\src\main.c"
   if errorlevel 1 goto failed
)

echo "Copying build files"
xcopy /Q /Y "%SRC_PATH%\makefile" "%DST_PATH%"
if errorlevel 1 goto failed
xcopy /Q /Y "%SRC_PATH%\make.bat" "%DST_PATH%"
if errorlevel 1 goto failed
xcopy /Q /Y "%SRC_PATH%\make-run.bat" "%DST_PATH%"
if errorlevel 1 goto failed
xcopy /Q /Y "%SRC_PATH%\make-clean.bat" "%DST_PATH%"
if errorlevel 1 goto failed
set "status=0"
goto finish

:failed
set "status=%errorlevel%"

:finish

if [%arg0:~2,1%]==[:] pause
endlocal & exit /b %status%
