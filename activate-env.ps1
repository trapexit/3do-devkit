if (-not $env:TDO_DEVKIT_PATH) {
    $env:TDO_DEVKIT_PATH = (Split-Path -Path $MyInvocation.MyCommand.Path -Parent)
    $env:PATH = "$($env:TDO_DEVKIT_PATH)\bin\compiler\win;$($env:TDO_DEVKIT_PATH)\bin\tools\win;$env:PATH"
}
