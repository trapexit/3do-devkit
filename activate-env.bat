@echo off
rem activate-env.bat
rem
rem Activates the 3DO development environment for the current Command Prompt session.
rem
rem Usage from an interactive Command Prompt:
rem
rem   activate-env.bat
rem
rem Usage from another batch file:
rem
rem   call activate-env.bat
rem
rem After activation, run:
rem
rem   deactivate-env
rem
rem This script intentionally does not use SETLOCAL at top level. Environment
rem changes must remain visible in the current cmd.exe session after the script exits.

if /i "%~1"=="deactivate" goto DeactivateFromMain
if /i "%~1"=="--deactivate" goto DeactivateFromMain
if /i "%~1"=="/deactivate" goto DeactivateFromMain

rem If this shell is already activated by this script, restore the exact saved
rem pre-activation environment before applying a new activation.
if /i "%__TDO_DEVKIT_ACTIVATION_STATE%"=="TDO_DEVKIT_ACTIVATE_ENV_STATE_V2_BAT" call :RestoreSnapshot

call :CaptureSnapshot

for %%I in ("%~dp0.") do set "__TDO_SCRIPT_DIR=%%~fI"
set "__TDO_PATH_FILE=%__TDO_SCRIPT_DIR%\.devkit-path"

if defined TDO_DEVKIT_PATH goto UseEnvironmentDevkitPath
if exist "%__TDO_PATH_FILE%" goto UseDotDevkitPath
set "__TDO_RESOLVED_DEVKIT_PATH=%__TDO_SCRIPT_DIR%"
goto DevkitPathResolved

:UseEnvironmentDevkitPath
set "__TDO_RAW_DEVKIT_PATH=%TDO_DEVKIT_PATH%"
call :ResolveDevkitPath __TDO_RAW_DEVKIT_PATH "%__TDO_SCRIPT_DIR%" "TDO_DEVKIT_PATH" require-full
if errorlevel 1 goto ActivationError
goto DevkitPathResolved

:UseDotDevkitPath
set "__TDO_RAW_DEVKIT_PATH="
for /f "usebackq delims=" %%A in ("%__TDO_PATH_FILE%") do if not defined __TDO_RAW_DEVKIT_PATH set "__TDO_RAW_DEVKIT_PATH=%%A"
call :ResolveDevkitPath __TDO_RAW_DEVKIT_PATH "%__TDO_SCRIPT_DIR%" ".devkit-path at %__TDO_PATH_FILE%"
if errorlevel 1 goto ActivationError
goto DevkitPathResolved

:DevkitPathResolved
if exist "%__TDO_RESOLVED_DEVKIT_PATH%\NUL" goto DevkitDirectoryExists
set "__TDO_ERROR_MESSAGE=Devkit path does not exist: %__TDO_RESOLVED_DEVKIT_PATH%"
goto ActivationError

:DevkitDirectoryExists
set "__TDO_COMPILER_PATH=%__TDO_RESOLVED_DEVKIT_PATH%\bin\compiler\win"
set "__TDO_TOOLS_PATH=%__TDO_RESOLVED_DEVKIT_PATH%\bin\tools\win"
set "__TDO_BUILDTOOLS_PATH=%__TDO_RESOLVED_DEVKIT_PATH%\bin\buildtools\win"
set "__TDO_EXPECTED_ARMCC_PATH=%__TDO_COMPILER_PATH%\armcc.exe"
call :NormalizePathVar __TDO_COMPILER_PATH
call :NormalizePathVar __TDO_TOOLS_PATH
call :NormalizePathVar __TDO_BUILDTOOLS_PATH
call :NormalizePathVar __TDO_EXPECTED_ARMCC_PATH

if exist "%__TDO_EXPECTED_ARMCC_PATH%" goto ExpectedCompilerExists
set "__TDO_ERROR_MESSAGE=Activation failed. Expected compiler not found: %__TDO_EXPECTED_ARMCC_PATH%"
goto ActivationError

:ExpectedCompilerExists
call :PrepareDeactivateCommand
if errorlevel 1 goto ActivationError

set "__TDO_FILTERED_ORIGINAL_PATH="
if defined __TDO_ACTIVATE_ORIGINAL_PATH set "__TDO_PATH_SPLIT_REMAINING=%__TDO_ACTIVATE_ORIGINAL_PATH%" & call :AppendOriginalPathEntries

set "TDO_DEVKIT_PATH=%__TDO_RESOLVED_DEVKIT_PATH%"
if defined __TDO_FILTERED_ORIGINAL_PATH goto SetPathWithOriginal
set "PATH=%__TDO_COMPILER_PATH%;%__TDO_TOOLS_PATH%;%__TDO_BUILDTOOLS_PATH%;%__TDO_DEACTIVATE_DIR%"
goto PathUpdated

:SetPathWithOriginal
set "PATH=%__TDO_COMPILER_PATH%;%__TDO_TOOLS_PATH%;%__TDO_BUILDTOOLS_PATH%;%__TDO_DEACTIVATE_DIR%;%__TDO_FILTERED_ORIGINAL_PATH%"
goto PathUpdated

:PathUpdated
rem Reject a DOSKEY macro named armcc because interactive cmd.exe would expand
rem that before normal PATH lookup.
set "__TDO_ARMCC_MACRO="
where doskey >nul 2>nul
if not errorlevel 1 for /f "tokens=1,* delims==" %%A in ('doskey /macros 2^>nul') do if /i "%%A"=="armcc" set "__TDO_ARMCC_MACRO=%%B"
if not defined __TDO_ARMCC_MACRO goto CheckWhereArmcc
set "__TDO_ERROR_MESSAGE=Activation failed. A DOSKEY macro named armcc exists and would shadow the devkit compiler in interactive cmd.exe."
goto ActivationError

:CheckWhereArmcc
set "__TDO_ARMCC_ACTUAL="
for /f "delims=" %%A in ('where armcc 2^>nul') do if not defined __TDO_ARMCC_ACTUAL set "__TDO_ARMCC_ACTUAL=%%~fA"

if defined __TDO_ARMCC_ACTUAL goto ArmccResolved
set "__TDO_ERROR_MESSAGE=Activation failed. armcc not found in PATH after activation."
goto ActivationError

:ArmccResolved
if /i "%__TDO_ARMCC_ACTUAL%"=="%__TDO_EXPECTED_ARMCC_PATH%" goto ArmccIsExpectedCompiler
set "__TDO_ERROR_MESSAGE=Activation failed. armcc resolves to '%__TDO_ARMCC_ACTUAL%', expected '%__TDO_EXPECTED_ARMCC_PATH%'."
goto ActivationError

:ArmccIsExpectedCompiler
set "__TDO_DEVKIT_ACTIVATION_STATE=TDO_DEVKIT_ACTIVATE_ENV_STATE_V2_BAT"
set "__TDO_ACTIVATE_ACTIVE_DEVKIT_PATH=%__TDO_RESOLVED_DEVKIT_PATH%"
set "__TDO_ACTIVATE_EXPECTED_ARMCC_PATH=%__TDO_EXPECTED_ARMCC_PATH%"
set "__TDO_ACTIVATE_DEACTIVATE_DIR=%__TDO_DEACTIVATE_DIR%"
set "__TDO_ACTIVATE_DEACTIVATE_SCRIPT=%__TDO_DEACTIVATE_SCRIPT%"

call :ClearTransientVariables

echo 3DO development environment activated. Run 'deactivate-env' to deactivate.
exit /b 0

:ActivationError
call :RestoreSnapshot
if defined __TDO_ERROR_MESSAGE goto ActivationErrorWithMessage
echo Error: Activation failed.
goto ActivationErrorAfterMessage

:ActivationErrorWithMessage
echo Error: %__TDO_ERROR_MESSAGE%

:ActivationErrorAfterMessage
if defined __TDO_RESOLVED_DEVKIT_PATH echo TDO_DEVKIT_PATH is: %__TDO_RESOLVED_DEVKIT_PATH%
if exist "%__TDO_PATH_FILE%" echo Note: .devkit-path was read from: %__TDO_PATH_FILE%
if exist "%__TDO_PATH_FILE%" echo If this path is wrong, update .devkit-path.
if defined __TDO_DEACTIVATE_SCRIPT if exist "%__TDO_DEACTIVATE_SCRIPT%" del "%__TDO_DEACTIVATE_SCRIPT%" >nul 2>nul
if defined __TDO_DEACTIVATE_DIR if exist "%__TDO_DEACTIVATE_DIR%\NUL" rmdir /s /q "%__TDO_DEACTIVATE_DIR%" >nul 2>nul
call :ClearTransientVariables
exit /b 1

:DeactivateFromMain
if /i "%__TDO_DEVKIT_ACTIVATION_STATE%"=="TDO_DEVKIT_ACTIVATE_ENV_STATE_V2_BAT" goto DeactivateFromMainActive
echo 3DO development environment not activated.
exit /b 1

:DeactivateFromMainActive
call :RestoreSnapshot
echo 3DO development environment deactivated.
exit /b 0

:CaptureSnapshot
if defined PATH goto CapturePathPresent
set "__TDO_ACTIVATE_HAD_ORIGINAL_PATH=0"
set "__TDO_ACTIVATE_ORIGINAL_PATH="
goto CaptureDevkitPath

:CapturePathPresent
set "__TDO_ACTIVATE_HAD_ORIGINAL_PATH=1"
set "__TDO_ACTIVATE_ORIGINAL_PATH=%PATH%"
goto CaptureDevkitPath

:CaptureDevkitPath
if defined TDO_DEVKIT_PATH goto CaptureDevkitPresent
set "__TDO_ACTIVATE_HAD_ORIGINAL_DEVKIT_PATH=0"
set "__TDO_ACTIVATE_ORIGINAL_DEVKIT_PATH="
exit /b 0

:CaptureDevkitPresent
set "__TDO_ACTIVATE_HAD_ORIGINAL_DEVKIT_PATH=1"
set "__TDO_ACTIVATE_ORIGINAL_DEVKIT_PATH=%TDO_DEVKIT_PATH%"
exit /b 0

:RestoreSnapshot
if "%__TDO_ACTIVATE_HAD_ORIGINAL_PATH%"=="1" goto RestoreSnapshotPathPresent
set "PATH="
goto RestoreSnapshotDevkit

:RestoreSnapshotPathPresent
set "PATH=%__TDO_ACTIVATE_ORIGINAL_PATH%"
goto RestoreSnapshotDevkit

:RestoreSnapshotDevkit
if "%__TDO_ACTIVATE_HAD_ORIGINAL_DEVKIT_PATH%"=="1" goto RestoreSnapshotDevkitPresent
set "TDO_DEVKIT_PATH="
goto RestoreSnapshotClearState

:RestoreSnapshotDevkitPresent
set "TDO_DEVKIT_PATH=%__TDO_ACTIVATE_ORIGINAL_DEVKIT_PATH%"
goto RestoreSnapshotClearState

:RestoreSnapshotClearState
if defined __TDO_ACTIVATE_DEACTIVATE_SCRIPT if exist "%__TDO_ACTIVATE_DEACTIVATE_SCRIPT%" del "%__TDO_ACTIVATE_DEACTIVATE_SCRIPT%" >nul 2>nul
if defined __TDO_ACTIVATE_DEACTIVATE_DIR if exist "%__TDO_ACTIVATE_DEACTIVATE_DIR%\NUL" rmdir /s /q "%__TDO_ACTIVATE_DEACTIVATE_DIR%" >nul 2>nul
call :ClearActivationState
exit /b 0

:ClearActivationState
set "__TDO_DEVKIT_ACTIVATION_STATE="
set "__TDO_ACTIVATE_HAD_ORIGINAL_PATH="
set "__TDO_ACTIVATE_ORIGINAL_PATH="
set "__TDO_ACTIVATE_HAD_ORIGINAL_DEVKIT_PATH="
set "__TDO_ACTIVATE_ORIGINAL_DEVKIT_PATH="
set "__TDO_ACTIVATE_ACTIVE_DEVKIT_PATH="
set "__TDO_ACTIVATE_EXPECTED_ARMCC_PATH="
set "__TDO_ACTIVATE_DEACTIVATE_DIR="
set "__TDO_ACTIVATE_DEACTIVATE_SCRIPT="
exit /b 0

:ResolveDevkitPath
rem %1: variable containing raw path
rem %2: base directory for relative paths
rem %3: source description for errors
rem %4: require-full to reject relative paths
call set "__TDO_CLEAN_PATH=%%%~1%%"
set "__TDO_CLEAN_PATH=%__TDO_CLEAN_PATH:"=%"
set "__TDO_CLEAN_PATH=%__TDO_CLEAN_PATH:'=%"
call :TrimSpacesVar __TDO_CLEAN_PATH
set "__TDO_CLEAN_PATH=%__TDO_CLEAN_PATH:/=\%"

if defined __TDO_CLEAN_PATH goto ResolveClassify
set "__TDO_ERROR_MESSAGE=%~3 is empty."
exit /b 1

:ResolveClassify
call :ClassifyPath "%__TDO_CLEAN_PATH%"
if "%__TDO_PATH_CLASS%"=="FULL" goto ResolveFullPath
if "%__TDO_PATH_CLASS%"=="ROOTED" goto ResolveAmbiguousRootedPath
if /i "%~4"=="require-full" goto ResolveRequiresFullPath
goto ResolveRelativePath

:ResolveFullPath
set "__TDO_RESOLVED_DEVKIT_PATH=%__TDO_CLEAN_PATH%"
call :NormalizePathVar __TDO_RESOLVED_DEVKIT_PATH
exit /b 0

:ResolveAmbiguousRootedPath
set "__TDO_ERROR_MESSAGE=%~3 contains an ambiguous rooted path: %__TDO_CLEAN_PATH%. Use a fully qualified path like C:\path\to\devkit, a UNC path, or a relative path."
exit /b 1

:ResolveRequiresFullPath
set "__TDO_ERROR_MESSAGE=%~3 must be a fully qualified path, not a relative path: %__TDO_CLEAN_PATH%"
exit /b 1

:ResolveRelativePath
set "__TDO_RESOLVED_DEVKIT_PATH=%~2\%__TDO_CLEAN_PATH%"
call :NormalizePathVar __TDO_RESOLVED_DEVKIT_PATH
exit /b 0

:ClassifyPath
set "__TDO_TEST_PATH=%~1"
set "__TDO_PATH_CLASS=RELATIVE"
if not defined __TDO_TEST_PATH exit /b 0
if "%__TDO_TEST_PATH:~0,2%"=="\\" set "__TDO_PATH_CLASS=FULL"& exit /b 0
if "%__TDO_TEST_PATH:~0,2%"=="//" set "__TDO_PATH_CLASS=FULL"& exit /b 0
if "%__TDO_TEST_PATH:~1,1%"==":" goto ClassifyDrivePath
goto ClassifyRootedOrRelative

:ClassifyDrivePath
if "%__TDO_TEST_PATH:~2,1%"=="\" set "__TDO_PATH_CLASS=FULL"& exit /b 0
if "%__TDO_TEST_PATH:~2,1%"=="/" set "__TDO_PATH_CLASS=FULL"& exit /b 0
set "__TDO_PATH_CLASS=ROOTED"
exit /b 0

:ClassifyRootedOrRelative
if "%__TDO_TEST_PATH:~0,1%"=="\" set "__TDO_PATH_CLASS=ROOTED"& exit /b 0
if "%__TDO_TEST_PATH:~0,1%"=="/" set "__TDO_PATH_CLASS=ROOTED"& exit /b 0
set "__TDO_PATH_CLASS=RELATIVE"
exit /b 0

:NormalizePathVar
call set "__TDO_NORMALIZED_PATH=%%%~1%%"
for %%I in ("%__TDO_NORMALIZED_PATH%") do set "__TDO_NORMALIZED_PATH=%%~fI"
call :TrimTrailingSlashesVar __TDO_NORMALIZED_PATH
set "%~1=%__TDO_NORMALIZED_PATH%"
exit /b 0

:TrimSpacesVar
call set "__TDO_TRIM_VALUE=%%%~1%%"
for /f "tokens=* delims= " %%A in ("%__TDO_TRIM_VALUE%") do set "__TDO_TRIM_VALUE=%%A"

:TrimSpacesVarLoop
if "%__TDO_TRIM_VALUE:~-1%"==" " set "__TDO_TRIM_VALUE=%__TDO_TRIM_VALUE:~0,-1%"& goto TrimSpacesVarLoop
set "%~1=%__TDO_TRIM_VALUE%"
exit /b 0

:TrimTrailingSlashesVar
call set "__TDO_TRIM_SLASH_VALUE=%%%~1%%"

:TrimTrailingSlashesVarLoop
if not defined __TDO_TRIM_SLASH_VALUE goto TrimTrailingSlashesVarDone
if not "%__TDO_TRIM_SLASH_VALUE:~-1%"=="\" if not "%__TDO_TRIM_SLASH_VALUE:~-1%"=="/" goto TrimTrailingSlashesVarDone
if "%__TDO_TRIM_SLASH_VALUE:~1,2%"==":\" if "%__TDO_TRIM_SLASH_VALUE:~3%"=="" goto TrimTrailingSlashesVarDone
if "%__TDO_TRIM_SLASH_VALUE:~1,2%"==":/" if "%__TDO_TRIM_SLASH_VALUE:~3%"=="" goto TrimTrailingSlashesVarDone
set "__TDO_TRIM_SLASH_VALUE=%__TDO_TRIM_SLASH_VALUE:~0,-1%"
goto TrimTrailingSlashesVarLoop

:TrimTrailingSlashesVarDone
set "%~1=%__TDO_TRIM_SLASH_VALUE%"
exit /b 0

:PrepareDeactivateCommand
set "__TDO_DEACTIVATE_BASE="
if defined TEMP set "__TDO_DEACTIVATE_BASE=%TEMP%"
if defined __TDO_DEACTIVATE_BASE goto DeactivateBaseSelected
if defined TMP set "__TDO_DEACTIVATE_BASE=%TMP%"
if defined __TDO_DEACTIVATE_BASE goto DeactivateBaseSelected
set "__TDO_DEACTIVATE_BASE=%__TDO_SCRIPT_DIR%"

:DeactivateBaseSelected
set "__TDO_DEACTIVATE_DIR=%__TDO_DEACTIVATE_BASE%\tdo-devkit-activate-env-%RANDOM%-%RANDOM%"
call :NormalizePathVar __TDO_DEACTIVATE_DIR
if exist "%__TDO_DEACTIVATE_DIR%\NUL" goto DeactivateDirExists
mkdir "%__TDO_DEACTIVATE_DIR%" >nul 2>nul
if exist "%__TDO_DEACTIVATE_DIR%\NUL" goto DeactivateDirExists
set "__TDO_ERROR_MESSAGE=Could not create deactivate command directory: %__TDO_DEACTIVATE_DIR%"
exit /b 1

:DeactivateDirExists
set "__TDO_DEACTIVATE_SCRIPT=%__TDO_DEACTIVATE_DIR%\deactivate-env.bat"
call :WriteDeactivateScript
if errorlevel 1 exit /b 1
if exist "%__TDO_DEACTIVATE_SCRIPT%" exit /b 0
set "__TDO_ERROR_MESSAGE=Could not create deactivate command: %__TDO_DEACTIVATE_SCRIPT%"
exit /b 1

:WriteDeactivateScript
> "%__TDO_DEACTIVATE_SCRIPT%" echo @echo off
if errorlevel 1 goto WriteDeactivateScriptFailed
>> "%__TDO_DEACTIVATE_SCRIPT%" echo rem Generated by activate-env.bat. Restores the saved pre-activation environment.
>> "%__TDO_DEACTIVATE_SCRIPT%" echo if /i "%%__TDO_DEVKIT_ACTIVATION_STATE%%"=="TDO_DEVKIT_ACTIVATE_ENV_STATE_V2_BAT" goto TdoDeactivateActive
>> "%__TDO_DEACTIVATE_SCRIPT%" echo echo 3DO development environment not activated.
>> "%__TDO_DEACTIVATE_SCRIPT%" echo exit /b 1
>> "%__TDO_DEACTIVATE_SCRIPT%" echo :TdoDeactivateActive
>> "%__TDO_DEACTIVATE_SCRIPT%" echo if "%%__TDO_ACTIVATE_HAD_ORIGINAL_PATH%%"=="1" goto TdoRestorePath
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "PATH="
>> "%__TDO_DEACTIVATE_SCRIPT%" echo goto TdoRestoreDevkit
>> "%__TDO_DEACTIVATE_SCRIPT%" echo :TdoRestorePath
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "PATH=%%__TDO_ACTIVATE_ORIGINAL_PATH%%"
>> "%__TDO_DEACTIVATE_SCRIPT%" echo :TdoRestoreDevkit
>> "%__TDO_DEACTIVATE_SCRIPT%" echo if "%%__TDO_ACTIVATE_HAD_ORIGINAL_DEVKIT_PATH%%"=="1" goto TdoRestoreDevkitPath
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "TDO_DEVKIT_PATH="
>> "%__TDO_DEACTIVATE_SCRIPT%" echo goto TdoClearActivationState
>> "%__TDO_DEACTIVATE_SCRIPT%" echo :TdoRestoreDevkitPath
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "TDO_DEVKIT_PATH=%%__TDO_ACTIVATE_ORIGINAL_DEVKIT_PATH%%"
>> "%__TDO_DEACTIVATE_SCRIPT%" echo :TdoClearActivationState
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "__TDO_DEVKIT_ACTIVATION_STATE="
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "__TDO_ACTIVATE_HAD_ORIGINAL_PATH="
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "__TDO_ACTIVATE_ORIGINAL_PATH="
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "__TDO_ACTIVATE_HAD_ORIGINAL_DEVKIT_PATH="
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "__TDO_ACTIVATE_ORIGINAL_DEVKIT_PATH="
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "__TDO_ACTIVATE_ACTIVE_DEVKIT_PATH="
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "__TDO_ACTIVATE_EXPECTED_ARMCC_PATH="
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "__TDO_ACTIVATE_DEACTIVATE_DIR="
>> "%__TDO_DEACTIVATE_SCRIPT%" echo set "__TDO_ACTIVATE_DEACTIVATE_SCRIPT="
>> "%__TDO_DEACTIVATE_SCRIPT%" echo echo 3DO development environment deactivated.
>> "%__TDO_DEACTIVATE_SCRIPT%" echo exit /b 0
exit /b 0

:WriteDeactivateScriptFailed
set "__TDO_ERROR_MESSAGE=Could not create deactivate command: %__TDO_DEACTIVATE_SCRIPT%"
exit /b 1

:AppendOriginalPathEntries
for /f "delims=; tokens=1*" %%A in ("%__TDO_PATH_SPLIT_REMAINING%") do (
    call :AppendOriginalPathEntry "%%~A"
    set "__TDO_PATH_SPLIT_REMAINING=%%B"
)
if defined __TDO_PATH_SPLIT_REMAINING goto AppendOriginalPathEntries
exit /b 0

:AppendOriginalPathEntry
set "__TDO_PATH_ENTRY=%~1"
if not defined __TDO_PATH_ENTRY exit /b 0
set "__TDO_PATH_ENTRY_CANON=%__TDO_PATH_ENTRY%"
call :NormalizePathVar __TDO_PATH_ENTRY_CANON
if /i "%__TDO_PATH_ENTRY_CANON%"=="%__TDO_DEACTIVATE_DIR%" exit /b 0
if /i "%__TDO_PATH_ENTRY_CANON%"=="%__TDO_COMPILER_PATH%" exit /b 0
if /i "%__TDO_PATH_ENTRY_CANON%"=="%__TDO_TOOLS_PATH%" exit /b 0
if /i "%__TDO_PATH_ENTRY_CANON%"=="%__TDO_BUILDTOOLS_PATH%" exit /b 0
if defined __TDO_FILTERED_ORIGINAL_PATH goto AppendOriginalPathEntryAfterExisting
set "__TDO_FILTERED_ORIGINAL_PATH=%__TDO_PATH_ENTRY%"
exit /b 0

:AppendOriginalPathEntryAfterExisting
set "__TDO_FILTERED_ORIGINAL_PATH=%__TDO_FILTERED_ORIGINAL_PATH%;%__TDO_PATH_ENTRY%"
exit /b 0

:ClearTransientVariables
set "__TDO_SCRIPT_DIR="
set "__TDO_PATH_FILE="
set "__TDO_RAW_DEVKIT_PATH="
set "__TDO_RESOLVED_DEVKIT_PATH="
set "__TDO_COMPILER_PATH="
set "__TDO_TOOLS_PATH="
set "__TDO_BUILDTOOLS_PATH="
set "__TDO_EXPECTED_ARMCC_PATH="
set "__TDO_ARMCC_ACTUAL="
set "__TDO_ARMCC_MACRO="
set "__TDO_DEACTIVATE_BASE="
set "__TDO_DEACTIVATE_DIR="
set "__TDO_DEACTIVATE_SCRIPT="
set "__TDO_FILTERED_ORIGINAL_PATH="
set "__TDO_PATH_ENTRY="
set "__TDO_PATH_ENTRY_CANON="
set "__TDO_PATH_SPLIT_REMAINING="
set "__TDO_CLEAN_PATH="
set "__TDO_ERROR_MESSAGE="
set "__TDO_TEST_PATH="
set "__TDO_PATH_CLASS="
set "__TDO_NORMALIZED_PATH="
set "__TDO_TRIM_VALUE="
set "__TDO_TRIM_SLASH_VALUE="
exit /b 0
