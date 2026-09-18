$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$PF86 = [Environment]::GetFolderPath('ProgramFilesX86')
$VsWhere = Join-Path $PF86 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $VsWhere)) { throw 'Visual Studio Installer / vswhere.exe was not found.' }

$VS = & $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $VS) { throw 'Visual Studio C++ build tools were not found.' }

$VcVars = Join-Path $VS 'VC\Auxiliary\Build\vcvars64.bat'
$CppWinRT = Get-ChildItem (Join-Path $PF86 'Windows Kits\10\Include') -Directory |
    Sort-Object Name -Descending |
    ForEach-Object { Join-Path $_.FullName 'cppwinrt' } |
    Where-Object { Test-Path (Join-Path $_ 'winrt\Windows.Media.Control.h') } |
    Select-Object -First 1

if (-not $CppWinRT) { throw 'No Windows SDK with C++/WinRT headers was found.' }

$Out = Join-Path $Root 'dist'
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$Source = Join-Path $Root 'src\main.cpp'
$Exe = Join-Path $Out 'MicMediaAutoPause.exe'

$cmd = '"{0}" && cl.exe /nologo /std:c++20 /EHsc /O2 /W4 /DUNICODE /D_UNICODE /I"{1}" "{2}" /Fe:"{3}" /link advapi32.lib windowsapp.lib' -f $VcVars,$CppWinRT,$Source,$Exe
& cmd.exe /d /s /c $cmd
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE." }

Copy-Item (Join-Path $Root 'config.ini') (Join-Path $Out 'config.ini') -Force
Copy-Item (Join-Path $Root 'scripts\install.ps1') (Join-Path $Out 'install.ps1') -Force
Copy-Item (Join-Path $Root 'scripts\stop.ps1') (Join-Path $Out 'stop.ps1') -Force
Copy-Item (Join-Path $Root 'scripts\uninstall.ps1') (Join-Path $Out 'uninstall.ps1') -Force
Copy-Item (Join-Path $Root 'README.md') (Join-Path $Out 'README.md') -Force
Copy-Item (Join-Path $Root 'LICENSE') (Join-Path $Out 'LICENSE') -Force
Write-Host "Built: $Exe"
