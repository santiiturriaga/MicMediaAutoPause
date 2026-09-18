$ErrorActionPreference = 'SilentlyContinue'
try {
    $Event = [Threading.EventWaitHandle]::OpenExisting('Local\MicMediaAutoPause.Stop')
    $Event.Set() | Out-Null
    $Event.Dispose()
    Write-Host 'Stop signal sent.'
} catch {
    Write-Host 'No running MicMediaAutoPause instance was found.'
}
