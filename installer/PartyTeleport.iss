#ifndef AppVersion
#define AppVersion "0.2.0"
#endif
[Setup]
AppId={{B688DCD7-E988-44F8-B685-C8C23FC18231}
AppName=Grim Dawn — Party Teleport
AppVersion={#AppVersion}
AppPublisher=Tor-del
AppPublisherURL=https://github.com/Tor-del/GrimDawn-PartyTeleport
AppSupportURL=https://github.com/Tor-del/GrimDawn-PartyTeleport/issues
DefaultDirName={code:FindGame}
DefaultGroupName=Grim Dawn Party Teleport
DisableProgramGroupPage=yes
DisableDirPage=no
AppendDefaultDirName=no
DirExistsWarning=no
UsePreviousAppDir=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=commandline
WizardStyle=modern
SetupIconFile=PartyTeleport.ico
OutputDir=dist
OutputBaseFilename=GrimDawn-PartyTeleport-{#AppVersion}-Setup-x64
Compression=lzma2
SolidCompression=yes
UninstallFilesDir={app}\mods\PartyTeleport\Uninstall
UninstallDisplayIcon={app}\mods\PartyTeleport\PartyTeleport.ico
CloseApplications=no
RestartApplications=no
SetupLogging=yes
MinVersion=10.0
VersionInfoVersion={#AppVersion}
VersionInfoDescription=Grim Dawn - Party Teleport Installer
LanguageDetectionMethod=none
UsePreviousLanguage=no
ShowLanguageDialog=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[CustomMessages]
russian.DesktopIcon=Создать ярлык на рабочем столе
english.DesktopIcon=Create a desktop shortcut
russian.Launch=Подключить Party Teleport к запущенной игре
english.Launch=Attach Party Teleport to the running game
russian.Attach=Подключить Party Teleport к запущенной игре
english.Attach=Attach Party Teleport to the running game
russian.Help=Инструкция Party Teleport
english.Help=Party Teleport instructions
russian.InvalidGame=Выберите папку Grim Dawn, содержащую x64\Grim Dawn.exe, x64\Game.dll и x64\Engine.dll.
english.InvalidGame=Select the Grim Dawn folder containing x64\Grim Dawn.exe, x64\Game.dll and x64\Engine.dll.
russian.Unsupported=Эта сборка Grim Dawn пока не поддерживается Party Teleport 0.2. Установка отменена без изменения игры.
english.Unsupported=This Grim Dawn build is not supported by Party Teleport 0.2. No game files were changed.
russian.InUse=Закройте Grim Dawn и окно Party Teleport вручную, затем повторите установку или удаление. Установщик не закрывает игру автоматически.
english.InUse=Close Grim Dawn and Party Teleport manually, then retry installation or removal. Setup will not close the game automatically.

[Tasks]
Name: "desktopicon"; Description: "{cm:DesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "..\build\PartyTeleport.exe"; DestDir: "{app}\mods\PartyTeleport"; Flags: ignoreversion
Source: "..\build\PartyTeleport.dll"; DestDir: "{app}\mods\PartyTeleport"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}\mods\PartyTeleport"; DestName: "README.txt"; Flags: ignoreversion
Source: "..\README.ru.md"; DestDir: "{app}\mods\PartyTeleport"; DestName: "README.ru.txt"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}\mods\PartyTeleport"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "..\vendor\minhook\LICENSE.txt"; DestDir: "{app}\mods\PartyTeleport"; DestName: "MinHook-LICENSE.txt"; Flags: ignoreversion
Source: "PartyTeleport.ico"; DestDir: "{app}\mods\PartyTeleport"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Grim Dawn Party Teleport\Grim Dawn — Party Teleport"; Filename: "{app}\mods\PartyTeleport\PartyTeleport.exe"; Parameters: "--language {code:AppLanguage}"; WorkingDir: "{app}"; IconFilename: "{app}\mods\PartyTeleport\PartyTeleport.ico"
Name: "{autoprograms}\Grim Dawn Party Teleport\Party Teleport (English)"; Filename: "{app}\mods\PartyTeleport\PartyTeleport.exe"; Parameters: "--language en"; WorkingDir: "{app}"; IconFilename: "{app}\mods\PartyTeleport\PartyTeleport.ico"
Name: "{autoprograms}\Grim Dawn Party Teleport\Party Teleport (Русский)"; Filename: "{app}\mods\PartyTeleport\PartyTeleport.exe"; Parameters: "--language ru"; WorkingDir: "{app}"; IconFilename: "{app}\mods\PartyTeleport\PartyTeleport.ico"
Name: "{autoprograms}\Grim Dawn Party Teleport\Instructions (English)"; Filename: "{app}\mods\PartyTeleport\README.txt"
Name: "{autoprograms}\Grim Dawn Party Teleport\Инструкция (Русский)"; Filename: "{app}\mods\PartyTeleport\README.ru.txt"
Name: "{autodesktop}\Grim Dawn — Party Teleport"; Filename: "{app}\mods\PartyTeleport\PartyTeleport.exe"; Parameters: "--language {code:AppLanguage}"; WorkingDir: "{app}"; IconFilename: "{app}\mods\PartyTeleport\PartyTeleport.ico"; Tasks: desktopicon

[InstallDelete]
Type: files; Name: "{autodesktop}\Grim Dawn — подключить PartyTeleport.lnk"
Type: files; Name: "{autoprograms}\Grim Dawn PartyTeleport\Подключить PartyTeleport к запущенной игре.lnk"
Type: files; Name: "{autoprograms}\Grim Dawn PartyTeleport\Attach PartyTeleport to the running game.lnk"
Type: files; Name: "{autoprograms}\Grim Dawn PartyTeleport\Инструкция PartyTeleport.lnk"
Type: files; Name: "{autoprograms}\Grim Dawn PartyTeleport\PartyTeleport instructions.lnk"

[Run]
Filename: "{app}\mods\PartyTeleport\PartyTeleport.exe"; Parameters: "--language {code:AppLanguage}"; WorkingDir: "{app}"; Description: "{cm:Launch}"; Flags: nowait postinstall skipifsilent unchecked runasoriginaluser

[Code]
function AppLanguage(Param: string): string;
begin
  if ActiveLanguage = 'russian' then Result := 'ru' else Result := 'en';
end;

const
  InvalidHandle = -1;
function CreateFileW(Name: string; Access, Share: LongWord; Security: Integer;
  Creation, Flags: LongWord; Template: Integer): Integer;
  external 'CreateFileW@kernel32.dll stdcall';
function CloseHandle(Handle: Integer): Boolean;
  external 'CloseHandle@kernel32.dll stdcall';

function GameFolder(const Path: string): Boolean;
begin
  Result := FileExists(AddBackslash(Path) + 'x64\Grim Dawn.exe') and
    FileExists(AddBackslash(Path) + 'x64\Game.dll') and FileExists(AddBackslash(Path) + 'x64\Engine.dll');
end;

function LibraryPath(const Line: string): string;
var P, Q: Integer; Tail: string;
begin
  Result := ''; P := Pos('"path"', Line);
  if P = 0 then Exit;
  Tail := Copy(Line, P + 6, Length(Line)); P := Pos('"', Tail);
  if P = 0 then Exit;
  Tail := Copy(Tail, P + 1, Length(Tail)); Q := Pos('"', Tail);
  if Q = 0 then Exit;
  Result := Copy(Tail, 1, Q - 1); StringChangeEx(Result, '\\', '\', True);
end;

function FindGame(Param: string): string;
var SteamPath, Candidate: string; Lines: TArrayOfString; I: Integer;
begin
  Result := ExpandConstant('{autopf32}\Steam\steamapps\common\Grim Dawn');
  if RegQueryStringValue(HKLM64, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 219990', 'InstallLocation', Candidate) and GameFolder(Candidate) then begin Result := Candidate; Exit; end;
  if RegQueryStringValue(HKLM32, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 219990', 'InstallLocation', Candidate) and GameFolder(Candidate) then begin Result := Candidate; Exit; end;
  if not RegQueryStringValue(HKCU, 'Software\Valve\Steam', 'SteamPath', SteamPath) then
    RegQueryStringValue(HKLM32, 'Software\Valve\Steam', 'InstallPath', SteamPath);
  if SteamPath = '' then Exit;
  Candidate := AddBackslash(SteamPath) + 'steamapps\common\Grim Dawn';
  if GameFolder(Candidate) then begin Result := Candidate; Exit; end;
  if LoadStringsFromFile(AddBackslash(SteamPath) + 'steamapps\libraryfolders.vdf', Lines) then
    for I := 0 to GetArrayLength(Lines) - 1 do begin
      Candidate := LibraryPath(Lines[I]);
      if Candidate <> '' then begin
        Candidate := AddBackslash(Candidate) + 'steamapps\common\Grim Dawn';
        if GameFolder(Candidate) then begin Result := Candidate; Exit; end;
      end;
    end;
end;

function InUse(const Name: string): Boolean;
var Handle: Integer;
begin
  Result := False;
  if not FileExists(Name) then Exit;
  { Opening for write detects mapped EXE/DLL files. No bytes are written. }
  Handle := CreateFileW(Name, $40000000, 7, 0, 3, 0, 0);
  if Handle = InvalidHandle then Result := True else CloseHandle(Handle);
end;

function ValidateTarget(const Path: string): string;
begin
  Result := '';
  if not GameFolder(Path) then begin Result := CustomMessage('InvalidGame'); Exit; end;
  try
    if (GetSHA256OfFile(AddBackslash(Path) + 'x64\Game.dll') <> 'cd7ecd7942d719f96b42aff4896f8c25d52152b4a41191f71a4a66ea48cecad2') or
       (GetSHA256OfFile(AddBackslash(Path) + 'x64\Engine.dll') <> '4c2ea14ce1c591b315cc29a8a96819a5c3c7e194adbd6f7eea208b9bb8a93153') then Result := CustomMessage('Unsupported');
  except Result := CustomMessage('InvalidGame'); end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var Error: string;
begin
  Result := True;
  if (CurPageID = wpSelectDir) and not WizardSilent then begin
    Error := ValidateTarget(WizardDirValue);
    if Error <> '' then begin MsgBox(Error, mbError, MB_OK); Result := False; end;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): string;
var Path: string;
begin
  Path := ExpandConstant('{app}'); Result := ValidateTarget(Path);
  if Result <> '' then Exit;
  if InUse(AddBackslash(Path) + 'mods\PartyTeleport\PartyTeleport.dll') or
     InUse(AddBackslash(Path) + 'mods\PartyTeleport\PartyTeleport.exe') then Result := CustomMessage('InUse');
end;

function InitializeUninstall(): Boolean;
begin
  Result := not (InUse(ExpandConstant('{app}\mods\PartyTeleport\PartyTeleport.dll')) or
    InUse(ExpandConstant('{app}\mods\PartyTeleport\PartyTeleport.exe')));
  if not Result then MsgBox(CustomMessage('InUse'), mbError, MB_OK);
end;
