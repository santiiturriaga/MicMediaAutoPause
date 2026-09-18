$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$PF86 = [Environment]::GetFolderPath('ProgramFilesX86')

& (Join-Path $Root 'build.ps1')

$Candidates = @(
    (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'),
    (Join-Path $PF86 'Inno Setup 6\ISCC.exe'),
    (Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe')
) | Where-Object { $_ -and (Test-Path $_) }

$ISCC = $Candidates | Select-Object -First 1
if (-not $ISCC) {
    throw 'Inno Setup 6 was not found. Install JRSoftware.InnoSetup, then run this script again.'
}

$Installer = Join-Path $Root 'installer\MicMediaAutoPause.iss'
& $ISCC $Installer
if ($LASTEXITCODE -ne 0) {
    throw "Installer build failed with exit code $LASTEXITCODE."
}

Write-Host "Built installer in: $(Join-Path $Root 'release')"
