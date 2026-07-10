; Modding Lexicon — Inno Setup Script
; Download Inno Setup 6: https://jrsoftware.org/isdl.php

#define AppName      "Modding Lexicon"
#define AppVersion   "1.2"
#define AppPublisher "mojabed"
#define AppURL       "https://github.com/mojabed/ModdingLexicon"
#define AppExeName   "ModdingLexicon.exe"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
AllowNoIcons=yes
OutputDir=installer
OutputBaseFilename=ModdingLexicon-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"

[Files]
; The release build output — point this to your build folder
Source: "out\build\release\ModdingLexicon.exe";      DestDir: "{app}"; Flags: ignoreversion
Source: "out\build\release\*.dll";                      DestDir: "{app}"; Flags: ignoreversion
Source: "out\build\release\*.qm";                       DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "out\build\release\QtWebEngineProcess.exe";     DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "resources\*";                DestDir: "{app}\resources"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "out\build\release\platforms\*";                DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "out\build\release\styles\*";                   DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "out\build\release\imageformats\*";             DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "out\build\release\sqldrivers\*";               DestDir: "{app}\sqldrivers"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "out\build\release\tls\*";                      DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "out\build\release\qml\*";                      DestDir: "{app}\qml"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "out\build\release\translations\*";             DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

; MSVC runtime — adjust path to your Visual Studio version
Source: "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.CRT\*.dll"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}";            Filename: "{app}\{#AppExeName}"
Name: "{group}\Uninstall {#AppName}";  Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}";      Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent
