#ifndef AppVersion
  #error AppVersion is required
#endif
#ifndef StageDir
  #error StageDir is required
#endif
#ifndef OutputDir
  #error OutputDir is required
#endif
[Setup]
AppId={{3582A89D-57AD-4B61-A3C1-C23442F8FBA3}
AppName=KBass
AppVersion={#AppVersion}
AppPublisher=Independent Audio
AppPublisherURL=https://github.com/voyagersquare/KBass
AppSupportURL=https://github.com/voyagersquare/KBass/issues
VersionInfoVersion={#AppVersion}.0
DefaultDirName={autopf}\Independent Audio\KBass
AppComments=VST3 bass instrument for Windows x64
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableWelcomePage=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
OutputDir={#OutputDir}
OutputBaseFilename=KBass-{#AppVersion}-Windows-x64-Setup
LicenseFile=..\LICENSE
SetupLogging=yes
CloseApplications=yes
CloseApplicationsFilter=*.exe,*.dll,*.vst3
RestartApplications=no
UninstallDisplayName=KBass
UninstallFilesDir={app}\Uninstall
[Languages]
Name: english; MessagesFile: compiler:Default.isl
[Files]
Source: "{#StageDir}\KBass.vst3\*"; DestDir: "{commoncf64}\VST3\KBass.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\README.md"; DestDir: "{app}\Documentation"; Flags: ignoreversion
Source: "{#StageDir}\SATURATION.md"; DestDir: "{app}\Documentation"; Flags: ignoreversion
Source: "{#StageDir}\LICENSE"; DestDir: "{app}\Licenses"; Flags: ignoreversion
Source: "{#StageDir}\ThirdParty\*"; DestDir: "{app}\Licenses"; Flags: ignoreversion recursesubdirs createallsubdirs
[Messages]
FinishedLabel=KBass has been installed.%n%nOpen your DAW, rescan VST3 plug-ins and look for Independent Audio > KBass.
[Code]
function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo,
  MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
begin
  Result := 'VST3 plug-in:' + NewLine + Space +
    ExpandConstant('{commoncf64}\VST3\KBass.vst3') + NewLine + NewLine +
    'Documentation and uninstaller:' + NewLine + Space +
    ExpandConstant('{app}') + NewLine + NewLine +
    'DAW manufacturer:' + NewLine + Space + 'Independent Audio';
end;
