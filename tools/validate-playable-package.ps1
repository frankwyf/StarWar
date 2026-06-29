param(
    [string]$DistPath = "dist"
)

$ErrorActionPreference = "Stop"

if (!(Test-Path $DistPath)) {
    throw "Package directory not found: $DistPath"
}

$exe = Join-Path $DistPath "StarWar.exe"
if (!(Test-Path $exe)) {
    throw "Missing playable executable: $exe"
}

$assets = Join-Path $DistPath "assets"
if (!(Test-Path $assets)) {
    throw "Missing assets directory: $assets"
}

$requiredAssets = @(
    "earth.png",
    "small.png",
    "medium.png",
    "large.png",
    "CREDITS.txt"
)

foreach ($name in $requiredAssets) {
    $path = Join-Path $assets $name
    if (!(Test-Path $path)) {
        throw "Missing required asset: $path"
    }
}

$configDir = Join-Path $DistPath "config"
$gameplayCfg = Join-Path $configDir "gameplay.cfg"
if (!(Test-Path $gameplayCfg)) {
    throw "Missing gameplay tuning config: $gameplayCfg"
}

Write-Host "Package validation passed."
Write-Host "- Executable: $exe"
Write-Host "- Assets verified: $($requiredAssets -join ', ')"