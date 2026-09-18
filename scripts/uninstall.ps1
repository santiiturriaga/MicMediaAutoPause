$ErrorActionPreference = 'SilentlyContinue'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $Root 'stop.ps1')
Start-Sleep -Milliseconds 500
Stop-ScheduledTask -TaskName 'MicMediaAutoPause'
Unregister-ScheduledTask -TaskName 'MicMediaAutoPause' -Confirm:$false
Write-Host 'Scheduled task removed. Files and logs were left in place.'
