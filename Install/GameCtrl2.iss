; -- Languages.iss --
; Demonstrates a multilingual installation.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

#define MyApplication "GameCtrl"
#define MyVersion "1.3"

[Setup]
AppName={cm:MyAppName}
AppId={#MyApplication}
AppVerName={cm:MyAppVerName,{#MyVersion}}
DefaultDirName={commonpf}\{#MyApplication}
DefaultGroupName={#MyApplication}
Uninstallable=yes
UninstallDisplayIcon={app}\GameCtrl2.exe
VersionInfoDescription=My Program Setup
VersionInfoProductName={#MyApplication}
VersionInfoVersion={#MyVersion}
OutputBaseFilename={#MyApplication}Setup
; Uncomment the following line to disable the "Select Setup Language"
; dialog and have it rely solely on auto-detection.
;ShowLanguageDialog=no
; If you want all languages to be listed in the "Select Setup Language"
; dialog, even those that can't be displayed in the active code page,
; uncomment the following line. Note: Unicode Inno Setup always displays
; all languages.
;ShowUndisplayableLanguages=yes

[Languages]
Name: en; MessagesFile: "compiler:Default.isl"
Name: fr; MessagesFile: "compiler:Languages\French.isl"

[Messages]
en.BeveledLabel=English
fr.BeveledLabel=French
                       
[CustomMessages]
en.MyDescription=A simple and powerfull game limiter
en.MyAppName=Game Control
en.MyAppVerName=Game Control %1
fr.MyDescription=Un limiteur de temps de jeu simple et puissant
fr.MyAppName=Game Control
fr.MyAppVerName=Game Control %1

[Files]
Source: "../Release/GameCtrl2.exe"; DestDir: "{app}"
Source: "../Release/GamePad2.dll"; DestDir: "{app}"
Source: "../Release/Patch2.exe"; DestDir: "{app}"; Flags: deleteafterinstall
Source: "Readme.txt"; DestDir: "{app}"; Languages: en; Flags: isreadme

[Icons]
Name: "{group}\{#MyApplication}"; Filename: "{app}\GameCtrl2.exe"

[Registry]
Root: HKLM; Subkey: "Software\GameCtrl"; Flags: uninsdeletekeyifempty
Root: HKLM; Subkey: "Software\GameCtrl"; ValueType: dword; ValueName: "CHRONO"; ValueData: 40; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\GameCtrl"; ValueType: dword; ValueName: "REINITCHRONO"; ValueData: 40; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\GameCtrl"; ValueType: dword; ValueName: "NBDAYSTOREINIT"; ValueData: 1; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\GameCtrl"; ValueType: dword; ValueName: "LOWDATETIME"; ValueData: "$cd0f8000"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\GameCtrl"; ValueType: dword; ValueName: "HIGHDATETIME"; ValueData: "$01d592a2"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\GameCtrl"; ValueType: dword; ValueName: "NBGAMES"; ValueData: 0; Flags: uninsdeletekey

[Run]
Filename: "{app}\Patch2.exe"; Parameters: "{app}\GameCtrl2.exe"
Filename: "{app}\GameCtrl2.exe"; Parameters: "--install"

[UninstallRun]
Filename: "{app}\GameCtrl2.exe"; Parameters: "--uninstall"

;[Code]
;procedure MyAfterInstall();
;var
;  ResultCode: Integer;
;begin
;  if not Exec(ExpandConstant('{app}\Patch2.exe'), '"'+ExpandConstant('{app}\GameCtrl2.exe')+'"', '', SW_SHOWNORMAL, ewWaitUntilTerminated, ResultCode) then begin
;    MsgBox('Installation source of GameCtrl is invalid:' + SysErrorMessage(ResultCode) + '.', mbError, MB_OK);
;  end
;end;
