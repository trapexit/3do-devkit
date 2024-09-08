@echo off

set arg0=%0
set "url=https://3dodev.com/_media/roms/panafz1.bin"
set "destination=C:\RetroArch-Win64\system\panafz1.bin"
set "targetDir=C:\RetroArch-Win64\system"

if not exist "%targetDir%" (
    echo Please install RetroArch first.
    if [%arg0:~2,1%]==[:] pause
    exit /b 1
)

bitsadmin /transfer panafz1.bin /download /priority high "%url%" "%destination%"

if exist "%destination%" (
    echo File downloaded successfully to %destination%
) else (
    echo Failed to download the file.
)

set "url=https://3dodev.com/_media/roms/panafz1j-kanji.bin"
set "destination=C:\RetroArch-Win64\system\panafz1-kanji.bin"

bitsadmin /transfer panafz1j-kanji.bin /download /priority high "%url%" "%destination%"

if exist "%destination%" (
    echo File downloaded successfully to %destination%
) else (
    echo Failed to download the file.
)

if [%arg0:~2,1%]==[:] pause
