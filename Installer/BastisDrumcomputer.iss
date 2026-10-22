; ------------------------------------------------------------------------------
; Bastis Drumcomputer - Windows-Installer (Inno Setup 6.3 oder neuer)
;
; Erzeugt eine einzelne Setup-Datei, die
;   - das VST3 nach  C:\Program Files\Common Files\VST3  kopiert und
;   - die Standalone-App nach  C:\Program Files\Basti\Bastis Drumcomputer
;     installiert (mit Startmenue-Eintrag, optional Desktop-Verknuepfung).
;
; Vorher das Projekt im Release-Modus bauen (siehe build-windows.bat), dann:
;   "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" Installer\BastisDrumcomputer.iss
; Ergebnis: Installer\Output\BastisDrumcomputer-Setup-<Version>.exe
; ------------------------------------------------------------------------------

#define AppName      "Bastis Drumcomputer"
#define AppPublisher "Basti"
#ifndef AppVersion
  #define AppVersion "0.1.1"
#endif
#ifndef BuildDir
  #define BuildDir "..\build\BastisDrumcomputer_artefacts\Release"
#endif

[Setup]
; AppId nie aendern, sonst erkennt Windows Updates nicht als dieselbe App
AppId={{73986DCF-0B82-498A-BB5F-05D83325747F}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppPublisher}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#AppName}.exe
OutputDir=Output
OutputBaseFilename=BastisDrumcomputer-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
; VST3 liegt in "Common Files", dafuer braucht es Admin-Rechte
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0

[Languages]
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full";   Description: "Alles installieren"
Name: "custom"; Description: "Benutzerdefiniert"; Flags: iscustom

[Components]
Name: "vst3";       Description: "VST3-Plugin (fuer DAWs wie Ableton, FL Studio, Cubase, Reaper)"; Types: full custom
Name: "standalone"; Description: "Standalone-App (ohne DAW spielbar)"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Components: standalone; Flags: unchecked

[Files]
; VST3 ist auf Windows ein Ordner (Bundle) - komplett kopieren
Source: "{#BuildDir}\VST3\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\{#AppName}.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Resources\Fonts\OFL-GreatVibes.txt"; DestDir: "{app}\Lizenzen"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Components: standalone
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppName}.exe"; Description: "{cm:LaunchProgram,{#AppName}}"; Components: standalone; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#AppName}.vst3"
