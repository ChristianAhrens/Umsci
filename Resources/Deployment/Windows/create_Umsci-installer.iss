#define MyAppName "Umsci"
#define MyAppExeName "Umsci.exe"
#define MyAppExePath "..\..\..\Builds\VisualStudio2022\x64\Release\App\Umsci.exe"
#define MyAppVersion GetVersionNumbersString("..\..\..\Builds\VisualStudio2022\x64\Release\App\Umsci.exe")
#define MyAppPublisher "Christian Ahrens"
#define MyAppURL "https://www.github.com/ChristianAhrens/Umsci"

[Setup]
AppId={{0923C12E-915B-4E77-92FB-187CB97E73A9}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
VersionInfoVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\..\..\LICENSE
InfoAfterFile=..\..\..\CHANGELOG.md
PrivilegesRequired=lowest
OutputBaseFilename="{#MyAppName}Setup_v{#MyAppVersion}"
SetupIconFile=..\..\..\Builds\VisualStudio2022\icon.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#MyAppExePath}"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[UninstallDelete]
Type: files; Name: "{userappdata}\{#MyAppName}\*";
Type: dirifempty; Name: "{userappdata}\{#MyAppName}\";

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

