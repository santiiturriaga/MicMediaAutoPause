# MicMediaAutoPause

**Microphone active -> all playing media pauses. Microphone released -> only media this app paused resumes.**

MicMediaAutoPause is a tiny native Windows utility that watches microphone use by configured applications and automatically manages Windows media sessions. By default it pauses **every currently playing media session Windows exposes**, so it is not tied to Spotify, a particular browser, or a fixed list of music apps.

If multiple apps are playing at once, they are paused independently and all sessions successfully paused by MicMediaAutoPause are remembered. If you manually resume one of them while the microphone is still active, MicMediaAutoPause does not immediately pause it again, and it will not issue another play command to that already-playing session when the microphone is released.

- Works with any app or browser that exposes a Windows GSMTC/SMTC media session.
- Handles multiple simultaneously playing media sessions.
- Resumes only sessions it successfully paused; media that was already paused stays paused.
- Native Windows executable: no Electron, browser extension, service, or bundled runtime.
- Starts automatically at sign-in when installed with the default installer option.
- Does not open the microphone, record audio, transcribe speech, or send telemetry.
- Can be completely removed from **Settings -> Apps -> Installed apps**.

## One-click install

Download **`MicMediaAutoPause-Setup-v1.2.0.exe`** from the latest GitHub Release and open it.

The installer:

1. Installs the app for your Windows user under `%LOCALAPPDATA%\Programs\MicMediaAutoPause`.
2. Enables **Start MicMediaAutoPause automatically when I sign in** by default. You can uncheck it during setup.
3. Starts the utility after installation.
4. Adds a normal **MicMediaAutoPause** entry to **Settings -> Apps -> Installed apps**.

Uninstalling from Windows stops the running utility, removes its auto-start entry, removes the installed program files, and deletes its generated log. No administrator access is required.

Public release binaries are currently unsigned, so Windows SmartScreen may show an **Unknown Publisher** warning. The full source and build scripts are public for users who prefer to build it themselves.

## Media behavior

With the default configuration, MicMediaAutoPause asks Windows for all current media sessions and pauses every session whose playback state is **Playing**. It stores each session for which the Windows pause request succeeds.

When microphone use ends, it checks those remembered sessions again. Only sessions that are still **Paused** are sent a play request. If a session has already been resumed, stopped, closed, or otherwise changed state, it is left alone.

That means, for example, if Spotify and a browser video are both playing, both can pause together and both can resume. If you manually resume Spotify while still using the microphone, Spotify is left playing while the other remembered session remains paused until microphone use ends.

## Configuration

`config.ini`:

```ini
[MicMediaAutoPause]
TriggerApps=msedge.exe
PauseAllMedia=1
ExcludeMediaApps=
MediaApps=
PollMs=150
ResumeDelayMs=350
Logging=1
```

- `TriggerApps`: comma-separated executable names whose microphone use triggers pausing.
- `PauseAllMedia=1`: manage every currently-playing Windows media session. This is the default.
- `ExcludeMediaApps`: optional comma-separated substrings to ignore while `PauseAllMedia=1`.
- `PauseAllMedia=0`: switch to allowlist mode.
- `MediaApps`: comma-separated substrings to manage when allowlist mode is enabled.
- `PollMs`: microphone-state polling interval.
- `ResumeDelayMs`: short debounce before resuming.
- `Logging`: `1` or `0`.

Example: manage everything except Discord and Teams:

```ini
PauseAllMedia=1
ExcludeMediaApps=discord,teams
```

Example: manage only Spotify and Edge:

```ini
PauseAllMedia=0
MediaApps=spotify,msedge
```

## Compatibility

MicMediaAutoPause controls **Windows media sessions**, not raw audio streams. Most modern music/video applications and major browsers expose media through Windows system media controls. An application that plays audio without creating a GSMTC/SMTC media session will not be visible to MicMediaAutoPause.

## Logs

`MicMediaAutoPause.log` is an ordinary UTF-8 text file next to the installed executable. Logging is deliberately sparse: startup/shutdown, microphone state transitions, successful pause/resume actions, and errors.

It does **not** continuously record polling results, microphone audio, speech, or browsing activity.

Set `Logging=0` in `config.ini` or launch with `--no-log` to disable it.

## Diagnostics

```powershell
.\MicMediaAutoPause.exe --status
.\MicMediaAutoPause.exe --foreground
.\MicMediaAutoPause.exe --version
.\MicMediaAutoPause.exe --stop
```

## Portable/manual install

If you do not want the installer, each release also includes a portable ZIP. The `scripts` folder in the repository contains Task Scheduler install/stop/uninstall helpers.

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

To build the one-click installer, install **Inno Setup 6** and run:

```powershell
.\build-installer.ps1
```

The installer source is `installer\MicMediaAutoPause.iss`, and the resulting setup executable is written to `release\`.

## Implementation note

Media playback control uses Windows Global System Media Transport Controls (GSMTC). Microphone-use detection reads the current per-application state that Windows stores under CapabilityAccessManager's microphone ConsentStore.

That registry state works on the Windows 10/11 systems this project targets, but Microsoft does not document that registry layout as a stable public API, so a future Windows update could require the detector to be adjusted.

The runtime intentionally uses a simple 150 ms poll. In local testing its idle CPU usage is below the resolution of a five-second process sample, so replacing it with a more complicated event-driven watcher would currently trade simplicity for no meaningful user-visible performance gain.

## Privacy

There is no network code. The program reads local Windows microphone capability state and uses local Windows media-control APIs.

## License

MIT.
