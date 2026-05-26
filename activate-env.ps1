# activate-env.ps1
#
# Activates the 3DO development environment for the current PowerShell session.
#
# Recommended usage:
#
#   . .\activate-env.ps1
#
# Dot-sourcing keeps the intent explicit. The script also keeps its helper
# functions private and defines only deactivate in the global session.

$__tdoActivationScriptDir = if ($PSScriptRoot) {
    $PSScriptRoot
}
elseif ($PSCommandPath) {
    Split-Path -Path $PSCommandPath -Parent
}
elseif ($MyInvocation.MyCommand.Path) {
    Split-Path -Path $MyInvocation.MyCommand.Path -Parent
}
else {
    (Get-Location).ProviderPath
}

try {
    & {
        param(
            [Parameter(Mandatory = $true)]
            [string]$ScriptDir
        )

        $stateVariableName = '__TDO_DEVKIT_ACTIVATION_STATE'
        $stateMarker = 'TDO_DEVKIT_ACTIVATE_ENV_STATE_V2'

        function Normalize-TdoPath {
            param(
                [Parameter(Mandatory = $true)]
                [AllowEmptyString()]
                [string]$PathText
            )

            # Remove accidental whitespace and surrounding quotes.
            $cleanPath = $PathText.Trim().Trim('"').Trim("'")

            if (-not $cleanPath) {
                return $cleanPath
            }

            # Convert to a full path where possible.
            # This also normalizes relative segments such as "..".
            try {
                $cleanPath = [System.IO.Path]::GetFullPath($cleanPath)
            }
            catch {
                # If GetFullPath fails, keep the cleaned text so the caller can report it.
            }

            # Remove trailing slashes, but do not damage roots such as C:\, /,
            # or a UNC root.
            $rootLength = 0

            try {
                $root = [System.IO.Path]::GetPathRoot($cleanPath)

                if ($root) {
                    $rootLength = $root.Length
                }
            }
            catch {
                $rootLength = 0
            }

            while (
                $cleanPath.Length -gt $rootLength -and
                ($cleanPath.EndsWith('\') -or $cleanPath.EndsWith('/'))
            ) {
                $cleanPath = $cleanPath.Substring(0, $cleanPath.Length - 1)
            }

            return $cleanPath
        }

        function Get-PathCompareKey {
            param(
                [Parameter(Mandatory = $true)]
                [AllowEmptyString()]
                [string]$PathText
            )

            # Used only for comparing PATH entries.
            # Windows paths are case-insensitive, so comparisons should be too.
            return (Normalize-TdoPath -PathText $PathText).ToLowerInvariant()
        }

        function Test-TdoPathFullyQualified {
            param(
                [Parameter(Mandatory = $true)]
                [string]$PathText
            )

            $cleanPath = $PathText.Trim().Trim('"').Trim("'")

            if (-not $cleanPath) {
                return $false
            }

            # Prefer the framework API where available. Windows PowerShell 5.1
            # may not have this method, so keep a conservative fallback below.
            try {
                return [System.IO.Path]::IsPathFullyQualified($cleanPath)
            }
            catch {
                # Fall through to explicit checks.
            }

            # Windows drive-qualified absolute path, e.g. C:\DevKit.
            if ($cleanPath -match '^[A-Za-z]:[\\/]') {
                return $true
            }

            # UNC or extended-length path, e.g. \\server\share or \\?\C:\DevKit.
            if ($cleanPath -match '^[\\/]{2}[^\\/]+[\\/][^\\/]+') {
                return $true
            }

            # POSIX-style absolute path for PowerShell Core on non-Windows hosts.
            if ([System.IO.Path]::DirectorySeparatorChar -eq '/' -and $cleanPath -match '^/') {
                return $true
            }

            return $false
        }

        function Resolve-TdoRootPath {
            param(
                [Parameter(Mandatory = $true)]
                [string]$RawPath,

                [Parameter(Mandatory = $true)]
                [string]$BaseDir,

                [Parameter(Mandatory = $true)]
                [string]$SourceDescription,

                [switch]$RequireFullyQualified
            )

            $cleanPath = $RawPath.Trim().Trim('"').Trim("'")

            if (-not $cleanPath) {
                throw "$SourceDescription is empty."
            }

            if (Test-TdoPathFullyQualified -PathText $cleanPath) {
                return Normalize-TdoPath -PathText $cleanPath
            }

            if ([System.IO.Path]::IsPathRooted($cleanPath)) {
                throw "$SourceDescription contains an ambiguous rooted path: $cleanPath. Use a fully qualified path like C:\path\to\devkit, a UNC path, or a relative path."
            }

            if ($RequireFullyQualified) {
                throw "$SourceDescription must be a fully qualified path, not a relative path: $cleanPath"
            }

            # Relative .devkit-path entries are resolved relative to the script
            # directory, not relative to the caller's current directory.
            return Normalize-TdoPath -PathText (Join-Path -Path $BaseDir -ChildPath $cleanPath)
        }

        function Get-TdoDevkitPath {
            param(
                [Parameter(Mandatory = $true)]
                [string]$ScriptDir
            )

            # If TDO_DEVKIT_PATH is already set to a non-empty value, honor it.
            # This lets callers explicitly override .devkit-path.
            if ((Test-Path -LiteralPath Env:\TDO_DEVKIT_PATH) -and $env:TDO_DEVKIT_PATH) {
                return Resolve-TdoRootPath `
                    -RawPath $env:TDO_DEVKIT_PATH `
                    -BaseDir $ScriptDir `
                    -SourceDescription 'TDO_DEVKIT_PATH' `
                    -RequireFullyQualified
            }

            # Look for .devkit-path next to this script.
            # Do not look in the caller's current working directory, because that can
            # cause the wrong devkit path to be used.
            $pathFile = Join-Path -Path $ScriptDir -ChildPath '.devkit-path'

            if (Test-Path -LiteralPath $pathFile) {
                # -Raw preserves newlines, so Trim() is important here.
                # Without Trim(), a normal trailing newline in .devkit-path can become
                # part of the path and break path construction.
                $rawPath = (Get-Content -LiteralPath $pathFile -Raw).Trim()

                return Resolve-TdoRootPath `
                    -RawPath $rawPath `
                    -BaseDir $ScriptDir `
                    -SourceDescription ".devkit-path at $pathFile"
            }

            # If no explicit path is provided, assume the devkit root is the directory
            # containing this script.
            return Normalize-TdoPath -PathText $ScriptDir
        }

        function New-TdoPathWithEntriesFirst {
            param(
                [Parameter(Mandatory = $true)]
                [string[]]$PathsToPrepend,

                [AllowNull()]
                [string]$ExistingPath
            )

            # Build a case-insensitive set of the devkit paths we are about to add.
            # These are removed from the existing PATH first so activation is
            # repeatable and does not create duplicate devkit entries.
            $prependPathKeys = New-Object 'System.Collections.Generic.HashSet[string]'

            foreach ($path in $PathsToPrepend) {
                if ($path) {
                    [void]$prependPathKeys.Add((Get-PathCompareKey -PathText $path))
                }
            }

            # Rebuild PATH:
            #   1. Start with the devkit paths.
            #   2. Then add the existing PATH entries.
            #   3. Skip existing entries that duplicate the devkit paths.
            #   4. Preserve unrelated PATH entries as much as possible.
            #
            # Deactivation restores the exact original PATH from a saved snapshot.
            $newPathEntries = New-Object 'System.Collections.Generic.List[string]'
            $seenPrependKeys = New-Object 'System.Collections.Generic.HashSet[string]'

            foreach ($path in $PathsToPrepend) {
                if (-not $path) {
                    continue
                }

                $key = Get-PathCompareKey -PathText $path

                if ($seenPrependKeys.Add($key)) {
                    [void]$newPathEntries.Add($path)
                }
            }

            if ($null -ne $ExistingPath) {
                foreach ($entry in ($ExistingPath -split ';')) {
                    $entryForCompare = $entry.Trim()

                    if (-not $entryForCompare) {
                        [void]$newPathEntries.Add($entry)
                        continue
                    }

                    $key = Get-PathCompareKey -PathText $entryForCompare

                    if ($prependPathKeys.Contains($key)) {
                        continue
                    }

                    [void]$newPathEntries.Add($entry)
                }
            }

            return [string]::Join(';', $newPathEntries.ToArray())
        }

        function Get-GlobalDeactivateEnvFunction {
            try {
                return Get-Item -LiteralPath 'Function:\Global:deactivate' -ErrorAction Stop
            }
            catch {
                return $null
            }
        }

        function Restore-GlobalDeactivateEnvFunction {
            param(
                [Parameter(Mandatory = $true)]
                [pscustomobject]$State
            )

            if ($State.HadOriginalDeactivateFunction) {
                Set-Item -LiteralPath 'Function:\Global:deactivate' -Value $State.OriginalDeactivateFunctionScriptBlock -Force
            }
            else {
                Remove-Item -LiteralPath 'Function:\Global:deactivate' -ErrorAction SilentlyContinue
            }
        }

        function Get-TdoEnvironmentSnapshot {
            $hadOriginalPath = Test-Path -LiteralPath Env:\PATH
            $originalPath = if ($hadOriginalPath) {
                (Get-Item -LiteralPath Env:\PATH).Value
            }
            else {
                $null
            }

            $hadOriginalDevkitPath = Test-Path -LiteralPath Env:\TDO_DEVKIT_PATH
            $originalDevkitPath = if ($hadOriginalDevkitPath) {
                (Get-Item -LiteralPath Env:\TDO_DEVKIT_PATH).Value
            }
            else {
                $null
            }

            $originalDeactivateFunction = Get-GlobalDeactivateEnvFunction
            $hadOriginalDeactivateFunction = $null -ne $originalDeactivateFunction
            $originalDeactivateFunctionScriptBlock = if ($hadOriginalDeactivateFunction) {
                $originalDeactivateFunction.ScriptBlock
            }
            else {
                $null
            }

            return [pscustomobject]@{
                Marker                                = $stateMarker
                HadOriginalPath                       = $hadOriginalPath
                OriginalPath                          = $originalPath
                HadOriginalDevkitPath                 = $hadOriginalDevkitPath
                OriginalDevkitPath                    = $originalDevkitPath
                HadOriginalDeactivateFunction         = $hadOriginalDeactivateFunction
                OriginalDeactivateFunctionScriptBlock = $originalDeactivateFunctionScriptBlock
                ActiveDevkitPath                      = $null
                AddedPathEntries                      = @()
                ExpectedArmccPath                     = $null
            }
        }

        function Restore-TdoEnvironmentSnapshot {
            param(
                [Parameter(Mandatory = $true)]
                [pscustomobject]$Snapshot
            )

            if ($Snapshot.HadOriginalPath) {
                Set-Item -LiteralPath Env:\PATH -Value $Snapshot.OriginalPath
            }
            else {
                Remove-Item -LiteralPath Env:\PATH -ErrorAction SilentlyContinue
            }

            if ($Snapshot.HadOriginalDevkitPath) {
                Set-Item -LiteralPath Env:\TDO_DEVKIT_PATH -Value $Snapshot.OriginalDevkitPath
            }
            else {
                Remove-Item -LiteralPath Env:\TDO_DEVKIT_PATH -ErrorAction SilentlyContinue
            }

            Remove-Variable -Name $stateVariableName -Scope Global -ErrorAction SilentlyContinue
            Restore-GlobalDeactivateEnvFunction -State $Snapshot
        }

        # If the script is run again while already activated, return the session to
        # the saved pre-activation state before applying the new activation. This
        # avoids stacking PATH edits and avoids using a stale active TDO_DEVKIT_PATH
        # as the next activation's input.
        $existingStateVariable = Get-Variable -Name $stateVariableName -Scope Global -ErrorAction SilentlyContinue

        if (
            $existingStateVariable -and
            $existingStateVariable.Value -and
            $existingStateVariable.Value.Marker -eq $stateMarker
        ) {
            Restore-TdoEnvironmentSnapshot -Snapshot $existingStateVariable.Value
        }

        $snapshot = Get-TdoEnvironmentSnapshot
        $resolvedDevkitPath = $null
        $pathFile = Join-Path -Path $ScriptDir -ChildPath '.devkit-path'

        try {
            # Resolve the devkit root from:
            #   1. Existing non-empty TDO_DEVKIT_PATH, or
            #   2. .devkit-path next to this script, or
            #   3. The script directory itself.
            $resolvedDevkitPath = Get-TdoDevkitPath -ScriptDir $ScriptDir

            if (-not (Test-Path -LiteralPath $resolvedDevkitPath -PathType Container)) {
                throw "Devkit path does not exist: $resolvedDevkitPath"
            }

            # These are the devkit directories that should be available on PATH.
            # They are intentionally added at the beginning of PATH so these tools win over
            # any similarly named tools already installed elsewhere.
            $compilerPath = Join-Path -Path $resolvedDevkitPath -ChildPath 'bin\compiler\win'
            $devkitPaths = @(
                $compilerPath,
                (Join-Path -Path $resolvedDevkitPath -ChildPath 'bin\tools\win'),
                (Join-Path -Path $resolvedDevkitPath -ChildPath 'bin\buildtools\win')
            )

            $expectedArmcc = Join-Path -Path $compilerPath -ChildPath 'armcc.exe'

            if (-not (Test-Path -LiteralPath $expectedArmcc -PathType Leaf)) {
                throw "Activation failed. Expected compiler not found: $expectedArmcc"
            }

            Set-Item -LiteralPath Env:\TDO_DEVKIT_PATH -Value $resolvedDevkitPath
            Set-Item -LiteralPath Env:\PATH -Value (New-TdoPathWithEntriesFirst -PathsToPrepend $devkitPaths -ExistingPath $snapshot.OriginalPath)

            # Verify that the command PowerShell will run is the compiler from this
            # devkit, not an alias, function, script, or unrelated executable.
            $armccCommand = Get-Command -Name armcc -ErrorAction SilentlyContinue | Select-Object -First 1
            $actualArmccPath = if ($armccCommand -and $armccCommand.CommandType -eq 'Application') {
                if ($armccCommand.Source) {
                    $armccCommand.Source
                }
                else {
                    $armccCommand.Definition
                }
            }
            else {
                $null
            }

            if ($armccCommand -and $armccCommand.CommandType -ne 'Application') {
                throw "Activation failed. 'armcc' resolves to a $($armccCommand.CommandType) named '$($armccCommand.Name)' before the devkit compiler."
            }

            if (
                -not $actualArmccPath -or
                (Get-PathCompareKey -PathText $actualArmccPath) -ne (Get-PathCompareKey -PathText $expectedArmcc)
            ) {
                $actualDisplay = if ($actualArmccPath) { $actualArmccPath } else { '<not found>' }
                throw "Activation failed. 'armcc' resolves to '$actualDisplay', expected '$expectedArmcc'."
            }

            $snapshot.ActiveDevkitPath = $resolvedDevkitPath
            $snapshot.AddedPathEntries = $devkitPaths
            $snapshot.ExpectedArmccPath = $expectedArmcc
            Set-Variable -Name $stateVariableName -Scope Global -Value $snapshot

            function global:deactivate {
                $stateVariable = Get-Variable -Name $stateVariableName -Scope Global -ErrorAction SilentlyContinue

                if (
                    -not $stateVariable -or
                    -not $stateVariable.Value -or
                    $stateVariable.Value.Marker -ne $stateMarker
                ) {
                    Write-Host '3DO development environment not activated.'
                    Remove-Item -LiteralPath 'Function:\Global:deactivate' -ErrorAction SilentlyContinue
                    return
                }

                $state = $stateVariable.Value

                if ($state.HadOriginalPath) {
                    Set-Item -LiteralPath Env:\PATH -Value $state.OriginalPath
                }
                else {
                    Remove-Item -LiteralPath Env:\PATH -ErrorAction SilentlyContinue
                }

                if ($state.HadOriginalDevkitPath) {
                    Set-Item -LiteralPath Env:\TDO_DEVKIT_PATH -Value $state.OriginalDevkitPath
                }
                else {
                    Remove-Item -LiteralPath Env:\TDO_DEVKIT_PATH -ErrorAction SilentlyContinue
                }

                Remove-Variable -Name $stateVariableName -Scope Global -ErrorAction SilentlyContinue

                if ($state.HadOriginalDeactivateFunction) {
                    Set-Item -LiteralPath 'Function:\Global:deactivate' -Value $state.OriginalDeactivateFunctionScriptBlock -Force
                }
                else {
                    Remove-Item -LiteralPath 'Function:\Global:deactivate' -ErrorAction SilentlyContinue
                }

                Write-Host '3DO development environment deactivated.'
            }

            Write-Host "3DO development environment activated. Run 'deactivate' to deactivate."
        }
        catch {
            Restore-TdoEnvironmentSnapshot -Snapshot $snapshot

            Write-Host "Error: $($_.Exception.Message)"

            if ($resolvedDevkitPath) {
                Write-Host "TDO_DEVKIT_PATH is: $resolvedDevkitPath"
            }

            if (Test-Path -LiteralPath $pathFile) {
                Write-Host "Note: .devkit-path was read from: $pathFile"
                Write-Host 'If this path is wrong, update .devkit-path.'
            }

            return
        }
    } $__tdoActivationScriptDir
}
finally {
    Remove-Variable -Name __tdoActivationScriptDir -ErrorAction SilentlyContinue
}
