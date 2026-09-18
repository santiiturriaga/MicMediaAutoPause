# MicMediaAutoPause

A tiny native Windows utility that pauses selected media when selected applications start using the microphone, then resumes **only the media it actually paused** when the microphone is released.

The original use case is ChatGPT dictation in Microsoft Edge: start talking and Spotify/YouTube pauses; stop dictating and playback resumes.

## What it does

1. Watches Windows' per-application microphone-use state for configured trigger applications.
2. On **not using mic -> using mic**, asks matching Windows media sessions to pause.
3. Remembers only sessions it successfully paused.
4. On **using mic -> not using mic**, resumes those remembered sessions if they are still paused.

It never opens the microphone, captures audio, transcribes anything, or sends telemetry.

## Logs

`MicMediaAutoPause.log` is an ordinary UTF-8 text file next to the executable. Logging is deliberately sparse: startup/shutdown, microphone state transitions, successful pause/resume actions, and errors. It does **not** continuously record polling results, microphone audio, or speech.

Set `Logging=0` in `config.ini` or launch with `--no-log` to disable it.

## Configuration

```ini
[MicMediaAutoPause]
TriggerApps=msedge.exe
MediaApps=spotify,msedge
PollMs=150
ResumeDelayMs=350
Logging=1
```

## Diagnostics

```powershell
.\MicMediaAutoPause.exe --status
.\MicMediaAutoPause.exe --foreground
```

## Install

The release folder contains `install.ps1`, `stop.ps1`, and `uninstall.ps1`. The installer creates a normal per-user Task Scheduler entry named **MicMediaAutoPause** that starts at logon. No administrator privileges or Windows service are required.

## Build

Requirements:
- Windows 10 1809+ or Windows 11
- Visual Studio C++ Build Tools
- A Windows SDK containing C++/WinRT headers

Run:

```powershell
.\build.ps1
```

The executable is written to `dist\MicMediaAutoPause.exe`.


## Implementation note

Media playback control uses Windows Global System Media Transport Controls (GSMTC). Microphone-use detection reads the current per-application state that Windows stores under CapabilityAccessManager's microphone ConsentStore. That registry state works on the Windows 10/11 systems this project targets, but Microsoft does not document that registry layout as a stable public API, so a future Windows update could require the detector to be adjusted.

## Privacy

There is no network code. The program reads local Windows microphone capability state and uses the local Windows Global System Media Transport Controls APIs.

## License

MIT.
