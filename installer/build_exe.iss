; Inno Setup Script for LookAway 20-20-20 Eye Care Utility
#define MyAppName "LookAway"
#define MyAppExeName "LookAway.exe"
#define MyAppPublisher "itsrajadarsh"
#define MyAppURL "https://github.com/itsrajadarsh/LookAway"

#ifndef MyAppVersion
  #define MyAppVersion "2.0.0"
#endif

#ifndef OutputBaseFilename
  #define OutputBaseFilename "LookAway-Setup-v" + MyAppVersion
#endif

#ifndef SourceDir
  #define SourceDir "..\build-windows\Release"
#endif

#ifndef OutputDir
  #define OutputDir "..\installer_output\windows"
#endif

[Setup]
AppId={{D3F9E10A-8A45-4E7B-9A8C-2B02C6A48191}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\LookAway
UninstallDisplayIcon={app}\app_icon.ico
SetupIconFile=..\resources\icons\app_icon.ico
WizardStyle=modern
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
DefaultGroupName={#MyAppName}
OutputBaseFilename={#OutputBaseFilename}
OutputDir={#OutputDir}
Compression=lzma2/ultra64
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "autostart"; Description: "Automatically launch LookAway when Windows starts"; GroupDescription: "Startup Options:"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "CMakeFiles,CMakeFiles\*,*.cxx.obj,*.cpp.obj,*.cpp,*.h,*.d,*.txt,*.cmake,*.lock,*.ninja,*.json,LookAway_autogen,LookAway_autogen\*"
Source: "..\resources\icons\app_icon.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "LookAway"; ValueData: """{app}\{#MyAppExeName}"" --minimized"; Flags: uninsdeletevalue; Tasks: autostart

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
