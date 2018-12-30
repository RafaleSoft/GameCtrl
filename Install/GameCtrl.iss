; -- Languages.iss --
; Demonstrates a multilingual installation.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

#define MyApplication "GameCtrl"
#define MyVersion "1.0"

[Setup]
AppName={cm:MyAppName}
AppId={#MyApplication}
AppVerName={cm:MyAppVerName,{#MyVersion}}
DefaultDirName={pf}\{#MyApplication}
DefaultGroupName={#MyApplication}
UninstallDisplayIcon={app}\GameCtrl.exe
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
Source: "../Release/GameCtrl.exe"; DestDir: "{app}"
Source: "Readme.txt"; DestDir: "{app}"; Languages: en; Flags: isreadme

[Icons]
Name: "{group}\{#MyApplication}"; Filename: "{app}\GameCtrl.exe"

