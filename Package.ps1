param([switch]$SkipBuild, [string]$CompilerPath)
$ErrorActionPreference = 'Stop'
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'Build-Installer.ps1') -CompilerPath $CompilerPath }
$version = (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'VERSION') -Raw).Trim()
$name = "GrimDawn-PartyTeleport-$version"
$stage = Join-Path $PSScriptRoot ('build\package-' + [guid]::NewGuid().ToString('N'))
$modStage = Join-Path $stage 'mods\PartyTeleport'
$dist = Join-Path $PSScriptRoot 'installer\dist'
New-Item -ItemType Directory -Force $modStage,$dist | Out-Null
foreach ($binary in @('PartyTeleport.exe','PartyTeleport.dll')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot ('build\'+$binary)) -Destination $modStage
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'installer\PartyTeleport.ico') -Destination $modStage
foreach ($doc in @('README.md','README.ru.md','LICENSE')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $doc) -Destination $modStage
}
New-Item -ItemType Directory -Force (Join-Path $modStage 'docs') | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'docs\preview.png') -Destination (Join-Path $modStage 'docs')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'vendor\minhook\LICENSE.txt') -Destination (Join-Path $modStage 'MinHook-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Launch-PartyTeleport.cmd') -Destination $stage
$zip = Join-Path $dist ($name+'-x64.zip')
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -Force
$setup = Join-Path $dist ($name+'-Setup-x64.exe')
if (-not (Test-Path -LiteralPath $setup)) { throw 'Build the installer before packaging.' }
$hashes = foreach ($artifact in @($setup,$zip)) {
    $hash = Get-FileHash -LiteralPath $artifact -Algorithm SHA256
    $hash.Hash.ToLowerInvariant() + '  ' + [IO.Path]::GetFileName($artifact)
}
$hashes | Set-Content -LiteralPath (Join-Path $dist 'SHA256SUMS.txt') -Encoding ascii
Write-Host ('Packaged installer, portable ZIP and SHA256SUMS.txt: '+$dist)
