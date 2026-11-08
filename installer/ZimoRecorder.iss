; =============================================================
;  ZIMO RECORDER - Windows installer (Inno Setup 6)
;
;  1. Build the app first:   cmake --build build
;  2. (Optional) put ffmpeg.exe in this folder - it will be bundled
;     so video/GIF recording works out of the box.
;  3. Open this file in Inno Setup and press Compile (Ctrl+F9).
;  The finished setup is written to  ..\release\
;
;  Installs for the current user only (no admin rights needed), which
;  also lets the in-app updater replace the exe later.
; =============================================================
#define AppName      "ZIMO RECORDER"
#define AppVersion   "1.0.0"
#define AppPublisher "Zimo"
#define AppExe       "ZimoRecorder.exe"

[Setup]
AppId={{B68E66BE-CD34-4F2D-AB45-30B831639F2C}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={localappdata}\Programs\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir=..\release
OutputBaseFilename=ZimoRecorder-Setup-{#AppVersion}
SetupIconFile=..\assets\icon.ico
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
; Setup asks the person to close the running app before installing/uninstalling
AppMutex=ZimoRecorderSingleInstanceMutex

[Languages]
Name: "english";    MessagesFile: "compiler:Default.isl"
Name: "german";     MessagesFile: "compiler:Languages\German.isl"
Name: "french";     MessagesFile: "compiler:Languages\French.isl"
Name: "spanish";    MessagesFile: "compiler:Languages\Spanish.isl"
Name: "russian";    MessagesFile: "compiler:Languages\Russian.isl"
Name: "turkish";    MessagesFile: "compiler:Languages\Turkish.isl"
Name: "italian";    MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese";   MessagesFile: "compiler:Languages\Japanese.isl"
Name: "dutch";      MessagesFile: "compiler:Languages\Dutch.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Shortcuts:"
Name: "autostart";   Description: "Start {#AppName} with &Windows (so the screenshot shortcuts work right away)"; GroupDescription: "Startup:"

[Files]
Source: "..\build\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion
#ifexist "ffmpeg.exe"
Source: "ffmpeg.exe"; DestDir: "{app}"; Flags: ignoreversion
#endif

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}";  Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Registry]
; "Start with Windows" - starts quietly in the tray (--background)
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "ZimoRecorder"; ValueData: """{app}\{#AppExe}"" --background"; Flags: uninsdeletevalue; Tasks: autostart
; App settings (update reminders) are removed on uninstall
Root: HKCU; Subkey: "Software\ZimoRecorder"; Flags: uninsdeletekey

[Run]
Filename: "{app}\{#AppExe}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\zimorecorder-lang.cfg"
