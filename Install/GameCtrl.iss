; -- Languages.iss --
; Demonstrates a multilingual installation.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

[Setup]
AppName={cm:GameCtrl}
AppId=GameCtrl
AppVerName={cm:GameCtrl,1.0}
DefaultDirName={pf}\{cm:GameCtrl}
DefaultGroupName={cm:MyAppName}
UninstallDisplayIcon={app}\GameCtrl.exe
VersionInfoDescription=My Program Setup
VersionInfoProductName=My Program
OutputDir=userdocs:Inno Setup Examples Output
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
Name: nl; MessagesFile: "compiler:Languages\Dutch.isl"
Name: de; MessagesFile: "compiler:Languages\German.isl"

[Messages]
en.BeveledLabel=English
nl.BeveledLabel=Nederlands
de.BeveledLabel=Deutsch

[CustomMessages]
en.MyDescription=My description
en.MyAppName=My Program
en.MyAppVerName=My Program %1
nl.MyDescription=Mijn omschrijving
nl.MyAppName=Mijn programma
nl.MyAppVerName=Mijn programma %1
de.MyDescription=Meine Beschreibung
de.MyAppName=Meine Anwendung
de.MyAppVerName=Meine Anwendung %1

[Files]
Source: "GameCtrl.exe"; DestDir: "{app}"
Source: "GameCtrl.chm"; DestDir: "{app}"; Languages: en
Source: "Readme.txt"; DestDir: "{app}"; Languages: en; Flags: isreadme

[Icons]
Name: "{group}\{cm:GameCtrl}"; Filename: "{app}\GameCtrl.exe"
Name: "{group}\{cm:UninstallProgram,{cm:GameCtrl}}"; Filename: "{uninstallexe}"

[Tasks]
; The following task doesn't do anything and is only meant to show [CustomMessages] usage
Name: mytask; Description: "{cm:MyDescription}"
