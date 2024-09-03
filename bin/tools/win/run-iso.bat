@echo off

if exist "C:\RetroArch-Win64\retroarch.exe" (
  C:\RetroArch-Win64\retroarch.exe --verbose -L opera_libretro.dll %1
) else if exist "C:\RetroArch\retroarch.exe" (
  C:\RetroArch\retroarch.exe --verbose -L opera_libretro.dll %1
) else (
  echo "Unable to find RetroArch. Download at https://retroarch.com"
)
