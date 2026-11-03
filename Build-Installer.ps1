param([string]$CompilerPath, [switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'Build.ps1') -StageOnly }
if (-not $CompilerPath) {
    $candidates = @(
        (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
        (Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe')
    )
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) { $candidates += $command.Source }
    $CompilerPath = $candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}
if (-not (Test-Path -LiteralPath $CompilerPath)) {
    throw 'Inno Setup compiler not found. Pass -CompilerPath with the full path to ISCC.exe.'
}
$version = (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'VERSION') -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'VERSION must contain a numeric x.y.z version.' }
& $CompilerPath "/DAppVersion=$version" (Join-Path $PSScriptRoot 'installer\PartyTeleport.iss')
if ($LASTEXITCODE -ne 0) { throw ('Inno Setup compilation failed: ' + $LASTEXITCODE) }
