& {
    $root = $PSScriptRoot
    $paths = @(
        [System.IO.Path]::Combine($root, 'bin', 'compiler', 'win')
        [System.IO.Path]::Combine($root, 'bin', 'tools', 'win')
        [System.IO.Path]::Combine($root, 'bin', 'buildtools', 'win')
    )

    foreach ($path in $paths) {
        if (-not (Test-Path -LiteralPath $path -PathType Container -ErrorAction Stop)) {
            throw "Activation failed: missing DevKit directory '$path'."
        }
    }
    $compiler = [System.IO.Path]::Combine($paths[0], 'armcc.exe')
    if (-not (Test-Path -LiteralPath $compiler -PathType Leaf -ErrorAction Stop)) {
        throw "Activation failed: missing DevKit compiler '$compiler'."
    }

    $removePaths = @($paths)
    if ($env:TDO_DEVKIT_PATH) {
        $previousRoot = $env:TDO_DEVKIT_PATH
        $removePaths += @(
            [System.IO.Path]::Combine($previousRoot, 'bin', 'compiler', 'win')
            [System.IO.Path]::Combine($previousRoot, 'bin', 'tools', 'win')
            [System.IO.Path]::Combine($previousRoot, 'bin', 'buildtools', 'win')
        )
    }
    $remainingPaths = @($env:PATH -split ';' | Where-Object { $_ -notin $removePaths })
    $newPath = ($paths + $remainingPaths) -join ';'

    $env:PATH = $newPath
    $env:TDO_DEVKIT_PATH = $root

    function global:deactivate-env {
        if (-not $env:TDO_DEVKIT_PATH) {
            Write-Host "3DO development environment not activated."
            return
        }

        $root = $env:TDO_DEVKIT_PATH
        $paths = @(
            [System.IO.Path]::Combine($root, 'bin', 'compiler', 'win')
            [System.IO.Path]::Combine($root, 'bin', 'tools', 'win')
            [System.IO.Path]::Combine($root, 'bin', 'buildtools', 'win')
        )
        $env:PATH = ($env:PATH -split ';' | Where-Object { $_ -notin $paths }) -join ';'
        Remove-Item Env:\TDO_DEVKIT_PATH -ErrorAction Stop
        Write-Host "3DO development environment deactivated."
    }

    Write-Host "3DO development environment activated. Run 'deactivate-env' to deactivate."
}
