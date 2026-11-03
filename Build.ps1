param([switch]$StageOnly)
$ErrorActionPreference = 'Stop'
$modFolder = $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsRoot) { throw 'Visual Studio C++ x64 build tools are required.' }
$vcvars = Join-Path $vsRoot 'VC/Auxiliary/Build/vcvars64.bat'
$buildFolder = Join-Path $modFolder 'build'
New-Item -ItemType Directory -Force $buildFolder | Out-Null
$buildScript = Join-Path $buildFolder 'compile.cmd'
@"
@echo off
call "$vcvars" >nul
if errorlevel 1 exit /b 1
cd /d "$buildFolder"
cl /nologo /O2 /MT /W3 /c "$modFolder\vendor\minhook\src\buffer.c" "$modFolder\vendor\minhook\src\hook.c" "$modFolder\vendor\minhook\src\trampoline.c" "$modFolder\vendor\minhook\src\hde\hde64.c"
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /O2 /MT /EHsc /W4 /LD "$modFolder\source\bridge.cpp" buffer.obj hook.obj trampoline.obj hde64.obj /link /OUT:"$buildFolder\PartyTeleport.dll" user32.lib
if errorlevel 1 exit /b 1
rc /nologo /fo app.res "$modFolder\source\app.rc"
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /O2 /MT /EHsc /W4 "$modFolder\source\app.cpp" "$modFolder\source\injector.cpp" app.res /link /SUBSYSTEM:WINDOWS /ENTRY:wmainCRTStartup /OUT:"$buildFolder\PartyTeleport.exe" user32.lib gdi32.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /O2 /MT /EHsc /W4 "$modFolder\source\tests.cpp" buffer.obj hook.obj trampoline.obj hde64.obj /link /OUT:"$buildFolder\tests.exe" user32.lib
if errorlevel 1 exit /b 1
"$buildFolder\tests.exe"
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /O2 /MT /EHsc /W4 "$modFolder\source\test_ui.cpp" /link /OUT:"$buildFolder\test_ui.exe" user32.lib gdi32.lib
if errorlevel 1 exit /b 1
"$buildFolder\test_ui.exe"
if errorlevel 1 exit /b 1
rem Compatibility checked by installer target validation.
exit /b %errorlevel%
"@ | Set-Content -LiteralPath $buildScript -Encoding ascii
& $buildScript
if ($LASTEXITCODE -ne 0) { throw "Build/check failed: $LASTEXITCODE" }

if (-not $StageOnly) { foreach ($binaryName in @('PartyTeleport.dll', 'PartyTeleport.exe')) {
    try { Copy-Item -LiteralPath (Join-Path $buildFolder $binaryName) -Destination (Join-Path $modFolder $binaryName) -Force -ErrorAction Stop }
    catch { Write-Host ("Built in build/: " + $binaryName + ("; installed copy was not updated: " + $_.Exception.Message)) }
} }
