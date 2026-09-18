# MicMediaAutoPause

**Talk in ChatGPT â†’ Spotify/YouTube pauses. Stop talking â†’ it resumes.**

MicMediaAutoPause is a tiny native Windows utility that watches microphone use by apps you choose and pauses matching media automatically. The important part: it remembers **only the media it paused**, so music that was already paused stays paused.

The default configuration is built around ChatGPT dictation in Microsoft Edge with Spotify or YouTube, but both trigger apps and media apps are configurable.

- Native Windows executable: no Electron, browser extension, service, or bundled runtime.
- Starts automatically at sign-in when installed with the default installer option.
- Does not open the microphone, record audio, transcribe speech, or send telemetry.
- Can be completely removed from **Settings â†’ Apps â†’ Installed apps**.

## One-click install

Download **`MicMediaAutoPause-Setup-v1.1.0.exe`** from the latest GitHub Release and open it.

The installer:

1. Installs the app for your Windows user under `%LOCALAPPDATA%\Programs\MicMediaAutoPause`.
2. Enables **Start MicMediaAutoPause automatically when I sign in** by default. You can uncheck it during setup.
3. Starts the utility after installation.
4. Adds a normal **MicMediaAutoPause** entry to **Settings â†’ Apps â†’ Installed apps**.

Uninstalling from Windows stops the running utility, removes its auto-start entry, removes the installed program files, and deletes its generated log. No administrator access is required.

Because public release binaries are currently unsigned, Windows SmartScreen may show an **Unknown Publisher** warning. The full source and build script are public, so advanced users can build it themselves instead.

## What it does

1. Watches Windows' per-application microphone-use state for configured trigger applications.
2. On **not using mic â†’ using mic**, asks matching Windows media sessions to pause.
3. Remembers only sessions it successfully paused.
4. On **using mic â†’ not using mic**, resumes those remembered sessions if they are still paused.

It never opens the microphone, captures audio, transcribes anything, or sends telemetry.

## Configuration

`config.ini`:

```ini
[MicMediaAutoPause]
TriggerApps=msedge.exe
MediaApps=spotify,msedge
PollMs=150
ResumeDelayMs=350
Logging=1
```

- `TriggerApps`: comma-separated executable names whose microphone use triggers pausing.
- `MediaApps`: comma-separated substrings matched against Windows media-session source IDs.
- `PollMs`: microphone-state polling interval.
- `ResumeDelayMs`: short debounce before resuming.
- `Logging`: `1` or `0`.

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

If you do not want the installer, the release also includes a portable ZIP. The `scripts` folder in the repository contains the Task Scheduler install/stop/uninstall helpers.

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
