@echo off

where retroarch >nul 2>nul
if %errorlevel% equ 0 (
  retroarch --verbose -L opera_libretro.dll %1
  goto :done
)

for %%D in (C D E F G H I J K L M N O P Q R S T U V W X Y Z) do (
  if exist "%%D:\RetroArch-Win64\retroarch.exe" (
    "%%D:\RetroArch-Win64\retroarch.exe" --verbose -L opera_libretro.dll %1
    goto :done
  )
  if exist "%%D:\RetroArch\retroarch.exe" (
    "%%D:\RetroArch\retroarch.exe" --verbose -L opera_libretro.dll %1
    goto :done
  )
)

echo Unable to find RetroArch. Download at https://retroarch.com
pause

:done
