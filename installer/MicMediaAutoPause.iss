#define MyAppName "MicMediaAutoPause"
#define MyAppVersion "1.1.0"
#define MyAppPublisher "Santiago Iturriaga"
#define MyAppURL "https://github.com/santiiturriaga/MicMediaAutoPause"
#define MyAppExeName "MicMediaAutoPause.exe"

[Setup]
AppId={{7CFE7158-64B8-44E2-9DAB-2AA369D7D2BC}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
AppUpdatesURL={#MyAppURL}/releases
DefaultDirName={localappdata}\Programs\MicMediaAutoPause
DefaultGroupName=MicMediaAutoPause
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
OutputDir=..\release
OutputBaseFilename=MicMediaAutoPause-Setup-v1.1.0
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\{#MyAppExeName}
CloseApplications=yes
RestartApplications=no
SetupLogging=yes

[Tasks]
Name: "autostart"; Description: "Start MicMediaAutoPause automatically when I sign in"; GroupDescription: "Startup:"; Flags: checkedonce

[Files]
Source: "..\dist\MicMediaAutoPause.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\config.ini"; DestDir: "{app}"; Flags: onlyifdoesntexist
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "MicMediaAutoPause"; ValueData: """{app}\{#MyAppExeName}"""; Flags: uninsdeletevalue; Tasks: autostart

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Start MicMediaAutoPause now"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\{#MyAppExeName}"; Parameters: "--stop"; Flags: runhidden waituntilterminated; RunOnceId: "StopMicMediaAutoPause"

[UninstallDelete]
Type: files; Name: "{app}\MicMediaAutoPause.log"
Type: dirifempty; Name: "{app}"

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  if FileExists(ExpandConstant('{app}\{#MyAppExeName}')) then
  begin
    Exec(ExpandConstant('{app}\{#MyAppExeName}'), '--stop', '', SW_HIDE,
      ewWaitUntilTerminated, ResultCode);
    Sleep(350);
  end;
  Result := '';
end;
