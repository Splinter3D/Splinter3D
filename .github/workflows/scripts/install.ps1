param(
  [string]$ProjectRoot = (Get-Location).Path
)

function Fail($msg) { Write-Error $msg; exit 1 }

Write-Host "install.ps1 starting (ProjectRoot: $ProjectRoot)"
$ProjectRoot = (Resolve-Path $ProjectRoot).Path
if (-not (Test-Path $ProjectRoot)) { Fail "Project root not found: $ProjectRoot" }

# Decide where to place vcpkg: sibling folder of the repo root
$vcpkgParent = Split-Path $ProjectRoot -Parent
$vcpkgDir = Join-Path $vcpkgParent 'vcpkg'

if (-not (Test-Path $vcpkgDir)) {
  Write-Host "Cloning vcpkg into $vcpkgDir"
  git clone https://github.com/microsoft/vcpkg.git $vcpkgDir
} else {
  Write-Host "vcpkg already present at $vcpkgDir"
}

Push-Location $vcpkgDir
$vcpkgExe = Join-Path $vcpkgDir 'vcpkg.exe'

if (-not (Test-Path $vcpkgExe)) {
  Write-Host "vcpkg.exe not found; attempting to download prebuilt binary"
  try {
    curl.exe -L -o $vcpkgExe "https://github.com/microsoft/vcpkg/releases/latest/download/vcpkg.exe"
  } catch {
    Write-Warning "curl download failed: $_"
  }
}

if (-not (Test-Path $vcpkgExe)) {
  Write-Host "Prebuilt vcpkg.exe not found; bootstrapping from source"
  try {
    & .\bootstrap-vcpkg.bat
  } catch {
    Write-Error "vcpkg bootstrap failed: $_"
    Pop-Location
    Fail "vcpkg bootstrap failed"
  }
}

# Integrate and ensure vcpkg works
try {
  & $vcpkgExe integrate install
} catch {
  Write-Warning "vcpkg integrate failed: $_"
}

Write-Host "vcpkg is ready at $vcpkgDir"
Pop-Location
Write-Host "install.ps1 finished"
