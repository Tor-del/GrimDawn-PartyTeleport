param([string]$GameRoot)
$ErrorActionPreference = 'Stop'
$version = (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'VERSION') -Raw).Trim()
$setupFile = Join-Path $PSScriptRoot ("installer\dist\GrimDawn-PartyTeleport-$version-Setup-x64.exe")
$testRoot = Join-Path $PSScriptRoot ('build\installer-checks-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null
function Run-RejectedInstall([string]$Target, [string]$Label, [string]$ExpectedText, [string]$Language = '') {
    $log = Join-Path $testRoot ($Label + '.log')
    # All fixtures must fail PrepareToInstall before files/shortcuts/registry entries are installed.
    $setupArgs = '/CURRENTUSER /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /NOCLOSEAPPLICATIONS /NOICONS /TASKS="" /DIR="' + $Target + '" /LOG="' + $log + '"'
    if ($Language) { $setupArgs += ' /LANG=' + $Language }
    $proc = Start-Process -FilePath $setupFile -ArgumentList $setupArgs -WindowStyle Hidden -PassThru
    if (-not $proc.WaitForExit(20000)) { $proc.Kill($true); throw ('Installer test timed out: ' + $Label) }
    $contents = Get-Content -LiteralPath $log -Raw
    if ($proc.ExitCode -ne 7 -or $contents -notmatch [regex]::Escape($ExpectedText) -or (Test-Path -LiteralPath (Join-Path $Target 'mods\PartyTeleport\PartyTeleport.exe'))) {
        throw ('Installer rejection test failed: ' + $Label + '; exit=' + $proc.ExitCode + '; log=' + $log)
    }
    Write-Host ('PASS: ' + $Label + '; setup exit=' + $proc.ExitCode)
}
Run-RejectedInstall (Join-Path $testRoot 'not-a-game-en') 'english-default' 'Select the Grim Dawn folder'
Run-RejectedInstall (Join-Path $testRoot 'not-a-game-ru') 'russian-language' 'Выберите папку Grim Dawn' 'russian'
$unsupported = Join-Path $testRoot 'unsupported-game'
New-Item -ItemType Directory -Path (Join-Path $unsupported 'x64') | Out-Null
foreach ($name in @('Grim Dawn.exe','Game.dll','Engine.dll')) {
    Set-Content -LiteralPath (Join-Path $unsupported ('x64\'+$name)) -Value 'Not a game binary.' -Encoding ascii
}
Run-RejectedInstall $unsupported 'unsupported-version' 'This Grim Dawn build is not supported'
if ($GameRoot) {
    $fixture = Join-Path $testRoot 'locked-module-fixture'
    New-Item -ItemType Directory -Path (Join-Path $fixture 'x64') | Out-Null
    foreach ($name in @('Grim Dawn.exe','Game.dll','Engine.dll')) {
        Copy-Item -LiteralPath (Join-Path $GameRoot ('x64\'+$name)) -Destination (Join-Path $fixture 'x64')
    }
    New-Item -ItemType Directory -Force (Join-Path $fixture 'mods\PartyTeleport') | Out-Null
    $dll = Join-Path $fixture 'mods\PartyTeleport\PartyTeleport.dll'
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'build\PartyTeleport.dll') -Destination $dll
    $heldFile = [IO.File]::Open($dll,'Open','Read','Read')
    try { Run-RejectedInstall $fixture 'locked-module' 'Close Grim Dawn and Party Teleport' }
    finally { $heldFile.Dispose() }
}
Write-Host ('Installer test logs: '+$testRoot)
