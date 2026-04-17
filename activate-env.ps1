function global:deactivate-env {
    if (-not $env:TDO_DEVKIT_PATH) {
        Write-Host "3DO development environment not activated."
        return
    }

    $env:PATH = $env:PATH -replace [regex]::Escape(";$env:TDO_DEVKIT_PATH\bin\compiler\win"), ""
    $env:PATH = $env:PATH -replace [regex]::Escape(";$env:TDO_DEVKIT_PATH\bin\tools\win"), ""
    $env:PATH = $env:PATH -replace [regex]::Escape(";$env:TDO_DEVKIT_PATH\bin\buildtools\win"), ""
    Remove-Item Env:\TDO_DEVKIT_PATH -ErrorAction SilentlyContinue

    Write-Host "3DO development environment deactivated."
    Remove-Item Function:deactivate-env -ErrorAction SilentlyContinue
}

if ($env:TDO_DEVKIT_PATH) {
    # Use existing
}
elseif (Test-Path ".devkit-path") {
    $env:TDO_DEVKIT_PATH = Get-Content ".devkit-path" -Raw
}
else {
    $env:TDO_DEVKIT_PATH = (Split-Path -Path $MyInvocation.MyCommand.Path -Parent)
}

$env:PATH = "$env:PATH;$env:TDO_DEVKIT_PATH\bin\compiler\win"
$env:PATH = "$env:PATH;$env:TDO_DEVKIT_PATH\bin\tools\win"
$env:PATH = "$env:PATH;$env:TDO_DEVKIT_PATH\bin\buildtools\win"

if (Get-Command armcc -ErrorAction SilentlyContinue) {
    Write-Host "3DO development environment activated. Run 'deactivate-env' to deactivate."
}
else {
    Write-Host "Error: Activation failed. armcc not found in PATH."
    if (Test-Path ".devkit-path") {
        Write-Host "Note: .devkit-path contains: $env:TDO_DEVKIT_PATH"
        Write-Host "If this path is wrong, update .devkit-path."
    }
}
