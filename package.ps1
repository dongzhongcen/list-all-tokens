param(
    [Parameter(Mandatory = $true)]
    [string]$QtRoot,
    [string]$BuildDir = "build",
    [string]$OutputDir = "dist"
)

$ErrorActionPreference = "Stop"
$qtBin = Join-Path $QtRoot "bin"
$cmakePrefix = $QtRoot

if (-not (Test-Path (Join-Path $qtBin "windeployqt.exe"))) {
    throw "windeployqt.exe not found under $qtBin"
}

cmake -S . -B $BuildDir -DCMAKE_PREFIX_PATH=$cmakePrefix -DCMAKE_BUILD_TYPE=Release
cmake --build $BuildDir --config Release

$builtExe = Join-Path $BuildDir "list-all-tokens.exe"
if (-not (Test-Path $builtExe)) { $builtExe = Join-Path $BuildDir "Release\list-all-tokens.exe" }
if (-not (Test-Path $builtExe)) { throw "Built executable not found under $BuildDir" }

$stage = Join-Path $OutputDir "list-all-tokens"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage -Force | Out-Null
Copy-Item $builtExe (Join-Path $stage "list-all-tokens.exe")
& (Join-Path $qtBin "windeployqt.exe") --release --no-translations (Join-Path $stage "list-all-tokens.exe")

$zip = Join-Path $OutputDir "list-all-tokens-windows.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
Write-Output "Created $zip"
