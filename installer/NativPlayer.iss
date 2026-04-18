#define MyAppName "NativPlayer"

#ifndef MyAppVersion
  #define MyAppVersion "0.0.0"
#endif

#ifndef PlayerExe
  #error "PlayerExe is required."
#endif

#ifndef ButtonStyleDir
  #error "ButtonStyleDir is required."
#endif

#ifndef LogoFile
  #error "LogoFile is required."
#endif

#ifndef SetupIconFile
  #error "SetupIconFile is required."
#endif

#ifndef ReadmeEnFile
  #error "ReadmeEnFile is required."
#endif

#ifndef ReadmeZhTwFile
  #error "ReadmeZhTwFile is required."
#endif

#ifndef ReadmeZhCnFile
  #error "ReadmeZhCnFile is required."
#endif

#ifndef OutputDir
  #error "OutputDir is required."
#endif

[Setup]
AppId={{7A10E0D4-89F3-4B4A-9452-27D7A2570C13}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher=NativPlayer Contributors
DefaultDirName={localappdata}\Programs\NativPlayer
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
UsePreviousLanguage=no
ShowLanguageDialog=yes
LanguageDetectionMethod=uilanguage
OutputDir={#OutputDir}
OutputBaseFilename=NativPlayer-Setup
SetupIconFile={#SetupIconFile}
UninstallDisplayIcon={app}\nativplayer.exe

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "zhTW"; MessagesFile: "compiler:Default.isl,languages\ChineseTraditional.isl"
Name: "zhCN"; MessagesFile: "compiler:Default.isl,languages\ChineseSimplified.isl"

[CustomMessages]
en.DesktopIcon=Create a desktop shortcut
zhTW.DesktopIcon=建立桌面捷徑
zhCN.DesktopIcon=建立桌面快捷方式
[Tasks]
Name: "desktopicon"; Description: "{cm:DesktopIcon}"; Flags: unchecked

[Files]
Source: "{#PlayerExe}"; DestDir: "{app}"; DestName: "nativplayer.exe"; Flags: ignoreversion
Source: "{#ButtonStyleDir}\*"; DestDir: "{app}\button-style"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#LogoFile}"; DestDir: "{app}"; DestName: "logo.png"; Flags: ignoreversion
Source: "{#ReadmeEnFile}"; DestDir: "{app}"; DestName: "README.en.md"; Flags: ignoreversion
Source: "{#ReadmeZhTwFile}"; DestDir: "{app}"; DestName: "README.zh-TW.md"; Flags: ignoreversion
Source: "{#ReadmeZhCnFile}"; DestDir: "{app}"; DestName: "README.zh-CN.md"; Flags: ignoreversion

[Icons]
Name: "{group}\NativPlayer"; Filename: "{app}\nativplayer.exe"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\NativPlayer"; Filename: "{app}\nativplayer.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\nativplayer.exe"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent unchecked

[UninstallDelete]
Type: files; Name: "{app}\nativplayer.default-language.txt"

[Code]
function GetAppLanguageCode(): string;
begin
  if ActiveLanguage = 'zhTW' then
    Result := 'zh-TW'
  else if ActiveLanguage = 'zhCN' then
    Result := 'zh-CN'
  else
    Result := 'en-US';
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  HintPath: string;
begin
  if CurStep <> ssPostInstall then
    exit;

  HintPath := ExpandConstant('{app}\nativplayer.default-language.txt');
  SaveStringToFile(HintPath, GetAppLanguageCode() + #13#10, False);
end;
