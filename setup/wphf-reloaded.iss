; Winprint HylaFAX Reloaded
; Copyright (C) 2011 Monti Lorenzo
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#define SrcApp "..\wphfgui\Release\wphfgui.exe"
#define FileVerStr GetFileVersion(SrcApp)
#define StripBuild(str VerStr) Copy(VerStr, 1, RPos(".", VerStr) - 1)
#define AppVerStr StripBuild(FileVerStr)
#define AppName "Winprint HylaFAX Reloaded"

[Setup]
AppId={{F64330DD-1138-4CB4-BF45-87F9168933F6}
AppName={#AppName}
AppVersion={#AppVerStr}
AppVerName={#AppName} {#AppVerStr}
AppPublisher=Monti Lorenzo
AppPublisherURL=http://wphf-reloaded.sourceforge.net/
AppSupportURL=http://wphf-reloaded.sourceforge.net/
AppUpdatesURL=http://wphf-reloaded.sourceforge.net/
UninstallDisplayName={#AppName} {#AppVerStr}
UninstallDisplayIcon={app}\wphfgui.exe
VersionInfoCompany=Monti Lorenzo
VersionInfoCopyright=Copyright © 2011 Monti Lorenzo
VersionInfoDescription={#AppName} setup program
VersionInfoProductName={#AppName}
VersionInfoVersion={#FileVerStr}
WizardImageFile=..\images\setup.bmp

CreateAppDir=yes
DefaultDirName={pf}\Winprint HylaFAX Reloaded
DefaultGroupName=Winprint HylaFAX Reloaded

OutputBaseFilename=wphf-reloaded-setup
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64
MinVersion=0,5.0

LicenseFile=gpl-3.0.rtf

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
en.errRegister=Error in port monitor registration!
en.errDeregister=Error in port monitor deregistration!
en.RemPrinter=Removing existing HylaFAX printer...
en.AddPrinter=Adding HylaFAX printer...
en.StartSpooler=Starting Spooler...
en.RegisteringMonitor=Registering port monitor...

[Files]
; x64 files
Source: ..\x64\release\wphfmon.dll; DestDir: "{sys}"; Flags: promptifolder replacesameversion; Check: Is_x64
Source: "..\x64\release\wphfmonUI.dll"; DestDir: "{sys}"; Flags: promptifolder replacesameversion; Check: Is_x64
; x86 files
Source: "..\win32\release\wphfmon.dll"; DestDir: "{sys}"; Flags: promptifolder replacesameversion; Check: Is_x86
Source: "..\win32\release\wphfmonUI.dll"; DestDir: "{sys}"; Flags: promptifolder replacesameversion; Check: Is_x86
; files common to both architectures
Source: "..\wphfgui\Release\wphfgui.exe"; DestDir: "{app}"; Flags: promptifolder replacesameversion
Source: "..\win32\release\regmon.exe"; DestDir: "{app}"; Flags: promptifolder replacesameversion
; Dell printer drivers
Source: "..\Dell\x86\*"; DestDir: "{app}\Dell"; MinVersion: 0.0,6.0; Flags: promptifolder replacesameversion recursesubdirs; Check: Is_x86
Source: "..\Dell\x64\*"; DestDir: "{app}\Dell"; MinVersion: 0.0,6.0; Flags: promptifolder replacesameversion recursesubdirs; Check: Is_x64
Source: "..\wphfgui\locale\*.mo"; DestDir: "{app}\locale"; Flags: recursesubdirs

[Tasks]
Name: printer; Description: "Install HylaFAX virtual printer"; Flags: checkedonce

[Icons]
Name: "{group}\Winprint HylaFAX GUI"; Filename: "{app}\wphfgui.exe"; WorkingDir: "{app}"

[Registry]
Root: HKLM; SubKey: "Software\Winprint HylaFAX Reloaded"; ValueType: string; ValueName: "InstallDir"; ValueData: "{app}"

[Run]
Filename: "{sys}\net.exe"; Parameters: "start spooler"; Flags: RunHidden; StatusMsg: {cm:StartSpooler}
Filename: "{app}\regmon.exe"; Parameters: "-r"; Flags: RunHidden; StatusMsg: {cm:RegisteringMonitor}
Filename: "{sys}\rundll32.exe"; Parameters: "PrintUI.dll,PrintUIEntry /dl /n ""HylaFAX"" /q"; StatusMsg: {cm:RemPrinter}; Tasks: printer
Filename: "{sys}\rundll32.exe"; Parameters: "PrintUI.dll,PrintUIEntry /if /b ""HylaFAX"" /f ""{win}\inf\ntprint.inf"" /r ""WPHF:"" /m ""Apple LaserWriter 16/600 PS"" /z /u /q"; OnlyBelowVersion: 0.0,6.0; StatusMsg: {cm:AddPrinter}; Tasks: printer
Filename: "{sys}\rundll32.exe"; Parameters: "PrintUI.dll,PrintUIEntry /if /b ""HylaFAX"" /f ""{app}\Dell\prndl001.inf"" /r ""WPHF:"" /m ""Dell 3100cn PS"" /z /u /q"; MinVersion: 0.0,6.0; StatusMsg: {cm:AddPrinter}; Tasks: printer

[UninstallRun]
Filename: "{sys}\net.exe"; Parameters: "start spooler"; Flags: RunHidden; StatusMsg: {cm:StartSpooler}

[UninstallDelete]
Type: filesandordirs; Name: "{commonappdata}\Winprint HylaFAX Reloaded\faxtmp"

[Code]
var
  bIsAnUpdate: Boolean;

{----------------------------------------------------------------------------------------}
function Is_x86: Boolean;
begin
  Result := (ProcessorArchitecture = paX86);
end;

{----------------------------------------------------------------------------------------}
function Is_x64: Boolean;
begin
  Result := (ProcessorArchitecture = paX64);
end;

{----------------------------------------------------------------------------------------}
function DestinationFilesExist: Boolean;
begin
  Result := FileExists(ExpandConstant('{sys}\wphfmon.dll')) and
            FileExists(ExpandConstant('{sys}\wphfmonUI.dll'));
end;

{----------------------------------------------------------------------------------------}
function InitializeSetup: Boolean;
begin
  bIsAnUpdate := DestinationFilesExist;
  Result := True;
end;

{----------------------------------------------------------------------------------------}
procedure CurStepChanged(CurStep: TSetupStep);
var
  rc: Integer;
begin
  case CurStep of
    ssInstall:
      begin
        //stop spooler since we're going to overwrite DLLs in use
        if bIsAnUpdate then
          Exec(ExpandConstant('{sys}\net.exe'), 'stop spooler', '', SW_HIDE, ewWaitUntilTerminated, rc);
      end;
  end;
end;

{----------------------------------------------------------------------------------------}
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  rc: Integer;
begin
  case CurUninstallStep of
    usUninstall:
      begin
        //make sure spooler is running
        Exec(ExpandConstant('{sys}\net.exe'), 'start spooler', '', SW_HIDE, ewWaitUntilTerminated, rc);
        //delete printer
        Exec(ExpandConstant('{sys}\rundll32.exe'), 'PrintUI.dll,PrintUIEntry /dl /n "HylaFAX" /q', '', SW_SHOW, ewWaitUntilTerminated, rc);
        //deregister monitor
        Exec(ExpandConstant('{app}\regmon.exe'), '-d', '', SW_HIDE, ewWaitUntilTerminated, rc);
        //stop spooler
        Exec(ExpandConstant('{sys}\net.exe'), 'stop spooler', '', SW_HIDE, ewWaitUntilTerminated, rc);
      end;
  end;
end;

