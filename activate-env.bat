@echo off
setlocal EnableExtensions
set "_tdo_delayed="
if "!"=="" set "_tdo_delayed=1"
setlocal DisableDelayedExpansion

set "_tdo_mode=activate"
if /i "%~1"=="deactivate" set "_tdo_mode=deactivate"
if /i "%~1"=="d" set "_tdo_mode=deactivate"
if "%_tdo_mode%"=="deactivate" goto :prepare-deactivation

rem Select and validate this installation before changing the caller's environment.
set "_tdo_root=%~dp0"
set "_tdo_compiler=%_tdo_root%bin\compiler\win"
set "_tdo_tools=%_tdo_root%bin\tools\win"
set "_tdo_buildtools=%_tdo_root%bin\buildtools\win"
if not exist "%_tdo_compiler%\." goto :missing-directory
if not exist "%_tdo_tools%\." goto :missing-directory
if not exist "%_tdo_buildtools%\." goto :missing-directory
if not exist "%_tdo_compiler%\armcc.exe" goto :missing-compiler
for %%F in ("%_tdo_compiler%\armcc.exe") do set "_tdo_compiler_attributes=%%~aF"
if /i "%_tdo_compiler_attributes:~0,1%"=="d" goto :missing-compiler
if not "%_tdo_root:~3%"=="" set "_tdo_root=%_tdo_root:~0,-1%"
goto :prepare-path

:prepare-deactivation
if not defined TDO_DEVKIT_PATH (
    echo 3DO development environment not activated.
    endlocal & endlocal
    exit /b 0
)
set "_tdo_root="
set "_tdo_compiler="
set "_tdo_tools="
set "_tdo_buildtools="

:prepare-path
setlocal EnableDelayedExpansion
set "_tdo_old_compiler="
set "_tdo_old_tools="
set "_tdo_old_buildtools="
if not defined TDO_DEVKIT_PATH goto :filter-path
set "_tdo_old_root=!TDO_DEVKIT_PATH!"
if "!_tdo_old_root:~-1!"=="\" set "_tdo_old_root=!_tdo_old_root:~0,-1!"
set "_tdo_old_compiler=!_tdo_old_root!\bin\compiler\win"
set "_tdo_old_tools=!_tdo_old_root!\bin\tools\win"
set "_tdo_old_buildtools=!_tdo_old_root!\bin\buildtools\win"

:filter-path
rem Delayed expansion treats PATH contents as data, including quotes, metacharacters, and !.
set "_tdo_remaining=!PATH!"
set "_tdo_entry="
set "_tdo_result="
set "_tdo_separator="

:scan-path
if not defined _tdo_remaining goto :last-entry
set "_tdo_char=!_tdo_remaining:~0,1!"
set "_tdo_compare=!_tdo_char:"=!"
if "!_tdo_compare!"==";" goto :next-entry
set "_tdo_entry=!_tdo_entry!!_tdo_char!"
set "_tdo_remaining=!_tdo_remaining:~1!"
goto :scan-path

:next-entry
set "_tdo_remaining=!_tdo_remaining:~1!"
set "_tdo_more=1"
goto :filter-entry

:last-entry
set "_tdo_more="

:filter-entry
rem Ignore quoting for comparisons only; keep the original entry when it is unrelated.
set "_tdo_compare=!_tdo_entry:"=!"
if defined _tdo_compiler if /i "!_tdo_compare!"=="!_tdo_compiler!" goto :skip-entry
if defined _tdo_tools if /i "!_tdo_compare!"=="!_tdo_tools!" goto :skip-entry
if defined _tdo_buildtools if /i "!_tdo_compare!"=="!_tdo_buildtools!" goto :skip-entry
if defined _tdo_old_compiler if /i "!_tdo_compare!"=="!_tdo_old_compiler!" goto :skip-entry
if defined _tdo_old_tools if /i "!_tdo_compare!"=="!_tdo_old_tools!" goto :skip-entry
if defined _tdo_old_buildtools if /i "!_tdo_compare!"=="!_tdo_old_buildtools!" goto :skip-entry
set "_tdo_result=!_tdo_result!!_tdo_separator!!_tdo_entry!"
set "_tdo_separator=;"

:skip-entry
set "_tdo_entry="
if defined _tdo_more goto :scan-path
if "%_tdo_mode%"=="deactivate" goto :prepare-export
set "_tdo_result=!_tdo_compiler!;!_tdo_tools!;!_tdo_buildtools!;!_tdo_result!"

:prepare-export
rem Restore quotes and percent signs through FOR variables after command parsing.
if defined _tdo_result set "_tdo_result=!_tdo_result:%%=%%~2!"
if defined _tdo_result set "_tdo_result=!_tdo_result:"=%%~3!"
if defined _tdo_root set "_tdo_root=!_tdo_root:%%=%%~2!"
if defined _tdo_root set "_tdo_root=!_tdo_root:"=%%~3!"
set "_tdo_replacements=%% """"

rem The trailing ! forces the native delayed-expansion caret pass when enabled.
if defined _tdo_delayed if defined _tdo_result set "_tdo_result=!_tdo_result:^=^^^^!"
if defined _tdo_delayed if defined _tdo_result set "_tdo_result=%_tdo_result:!=^^^!%" !
if defined _tdo_delayed if defined _tdo_root set "_tdo_root=!_tdo_root:^=^^^^!"
if defined _tdo_delayed if defined _tdo_root set "_tdo_root=%_tdo_root:!=^^^!%" !
if "%_tdo_mode%"=="deactivate" goto :export-deactivation

for /f "tokens=1,2" %%2 in ("!_tdo_replacements!") do (
    endlocal & endlocal & endlocal
    set "PATH=%_tdo_result%" !
    set "TDO_DEVKIT_PATH=%_tdo_root%" !
)
echo 3DO development environment activated. Run 'activate-env.bat deactivate' to deactivate.
exit /b 0

:export-deactivation
for /f "tokens=1,2" %%2 in ("!_tdo_replacements!") do (
    endlocal & endlocal & endlocal
    set "PATH=%_tdo_result%" !
    set "TDO_DEVKIT_PATH="
)
echo 3DO development environment deactivated.
exit /b 0

:missing-directory
echo Error: Activation failed. Missing compiler, tools, or buildtools directory in "%_tdo_root%". 1>&2
endlocal & endlocal
exit /b 1

:missing-compiler
echo Error: Activation failed. Missing DevKit compiler "%_tdo_compiler%\armcc.exe". 1>&2
endlocal & endlocal
exit /b 1
