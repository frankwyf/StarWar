param(
    [string]$Configuration = "debug",
    [string]$Platform = "x64",
    [switch]$SkipSmoke
)

$ErrorActionPreference = "Stop"

Write-Host "== Full validation start =="

msbuild lines-test\lines-test.vcxproj /m /p:Configuration=$Configuration /p:Platform=$Platform
msbuild triangles-test\triangles-test.vcxproj /m /p:Configuration=$Configuration /p:Platform=$Platform
msbuild main\main.vcxproj /m /p:Configuration=$Configuration /p:Platform=$Platform

$linesExe = "bin/lines-test-$Configuration-$Platform-msc-v145.exe"
$triExe = "bin/triangles-test-$Configuration-$Platform-msc-v145.exe"
if (!(Test-Path $linesExe)) { throw "Missing $linesExe" }
if (!(Test-Path $triExe)) { throw "Missing $triExe" }
& $linesExe --reporter compact
& $triExe --reporter compact

$mainExe = Get-ChildItem -Path bin -Filter "main-$Configuration-$Platform-*.exe" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $mainExe) { throw "Missing main executable" }

& $mainExe.FullName --selftest_assets

if (-not $SkipSmoke) {
    if (Test-Path artifacts) { Remove-Item artifacts -Recurse -Force }
    & $mainExe.FullName --smoketest=0.8 --geometry=960x540 --fbshift=0 --seed=42
    if (!(Test-Path artifacts\smoketest-last-frame.ppm)) { throw "Smoke frame not generated" }
} else {
    Write-Host "Smoke test skipped by flag."
}

if (Test-Path dist) { Remove-Item dist -Recurse -Force }
New-Item -ItemType Directory -Path dist | Out-Null
Copy-Item $mainExe.FullName dist/StarWar.exe
Copy-Item -Recurse -Force assets dist/assets
if (Test-Path config) { Copy-Item -Recurse -Force config dist/config }
powershell -ExecutionPolicy Bypass -File tools/validate-playable-package.ps1 -DistPath dist

Write-Host "== Full validation complete =="