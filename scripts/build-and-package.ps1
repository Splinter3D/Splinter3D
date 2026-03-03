<#
.SYNOPSIS
  Build and package the project on Windows runners.

.DESCRIPTION
  Bootstraps vcpkg if needed, installs manifest dependencies, configures CMake with the
  vcpkg toolchain, builds Release, runs install into a staging directory and creates a
  ZIP artifact containing the staging tree. Produces a SHA256 checksum file for the ZIP.

.PARAMETER OutDir
  Directory where output artifacts (ZIP, checksum) will be written.

.PARAMETER VcpkgDir
  Path to vcpkg (will clone and bootstrap if missing). Default: ./vcpkg

.PARAMETER RepoRoot
  Root of the repository. Default: current working directory.

.PARAMETER Triplet
  vcpkg triplet to use (default x64-windows).

.PARAMETER InstallPrefix
  Install prefix (staging directory). Default: <OutDir>/staging

.PARAMETER UseCPack
  If specified, run `cpack` in the build directory to create installer(s) (requires NSIS).

EXAMPLE
  pwsh -File ./scripts/build-and-package.ps1 -OutDir C:\actions\output -VcpkgDir C:\vcpkg
#>

param(
    [Parameter(Mandatory=$true)] [string]$OutDir,
    [string]$VcpkgDir = "$PSScriptRoot\..\vcpkg",
    [string]$RepoRoot = (Get-Location).Path,
    [string]$Triplet = "x64-windows",
    [string]$InstallPrefix = "",
    [switch]$UseCPack
)

function Fail($msg) {
    Write-Error $msg
    exit 1
}

Write-Host "build-and-package.ps1 starting"
Write-Host "RepoRoot: $RepoRoot"
Write-Host "VcpkgDir: $VcpkgDir"
Write-Host "Triplet: $Triplet"

if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Force -Path $OutDir | Out-Null }

if (-not $InstallPrefix) { $InstallPrefix = Join-Path $OutDir "staging" }

# Ensure vcpkg exists and is bootstrapped (clone full repo and bootstrap like Windows-Install)
if (-not (Test-Path $VcpkgDir)) {
  $parentDir = Split-Path $RepoRoot -Parent
  Set-Location $parentDir
  Write-Host "Cloning vcpkg into $parentDir\vcpkg..."
  git clone https://github.com/microsoft/vcpkg.git || Fail "Failed to clone vcpkg"
  $VcpkgDir = Join-Path $parentDir 'vcpkg'
}

Push-Location $VcpkgDir
Write-Host "Bootstrapping vcpkg..."
& .\bootstrap-vcpkg.bat || Fail "vcpkg bootstrap failed"
& .\vcpkg integrate install || Write-Warning "vcpkg integrate install failed"

Write-Host "Installing vcpkg dependencies (manifest-aware)..."
try {
  $vcpkgExe = (Join-Path $VcpkgDir 'vcpkg.exe')
  if (Test-Path (Join-Path $RepoRoot 'vcpkg.json')) {
    # Check if this vcpkg supports manifest mode
    $help = & $vcpkgExe help install 2>&1 | Out-String
    if ($help -match '--manifest') {
      Write-Host "Found vcpkg.json and vcpkg supports manifest mode; running manifest install"
      Push-Location $RepoRoot
      & $vcpkgExe install --manifest
      if ($LASTEXITCODE -ne 0) { Write-Warning "vcpkg manifest install failed with exit code $LASTEXITCODE; continuing" }
      Pop-Location
    } else {
      Write-Warning "vcpkg executable does not support '--manifest'; skipping manifest install."
    }
  } else {
    Write-Host "No vcpkg.json found; skipping manifest install. If you need classic installs, list ports explicitly."
  }
} catch {
  Write-Warning "vcpkg install step failed; continuing. Error: $_"
}
Pop-Location

# Prepare build directory
$buildDir = Join-Path $RepoRoot "build"
if (Test-Path $buildDir) { Remove-Item -Recurse -Force $buildDir }
New-Item -ItemType Directory -Path $buildDir | Out-Null

$toolchain = Join-Path $VcpkgDir "scripts\buildsystems\vcpkg.cmake"
if (-not (Test-Path $toolchain)) { Fail "vcpkg toolchain not found at $toolchain" }

Write-Host "Configuring CMake with generator fallback..."
$cmakeExe = "cmake"
$generators = @(
  @{ Name = 'Visual Studio 17 2022'; Args = @('-G', 'Visual Studio 17 2022', '-A', 'x64') },
  @{ Name = 'Visual Studio 16 2019'; Args = @('-G', 'Visual Studio 16 2019', '-A', 'x64') },
  @{ Name = 'Ninja'; Args = @('-G', 'Ninja') }
)

$selectedGenerator = $null
# verify toolchain exists
if (-not (Test-Path $toolchain)) {
    Write-Warning "vcpkg toolchain file not found at $toolchain"
    Write-Warning "If you expect a full vcpkg repo here, ensure you cloned the repository (not only downloaded vcpkg.exe)."
    Fail "vcpkg toolchain file missing"
}

foreach ($gen in $generators) {
  $sanitized = ($gen.Name -replace '[^A-Za-z0-9_-]', '_')
  $candidateBuild = Join-Path $RepoRoot ("build_$sanitized")
  if (Test-Path $candidateBuild) { Remove-Item -Recurse -Force $candidateBuild -ErrorAction SilentlyContinue }
  New-Item -ItemType Directory -Path $candidateBuild | Out-Null

  $cmakeArgs = @('-S', $RepoRoot, '-B', $candidateBuild) + $gen.Args + @('-DCMAKE_TOOLCHAIN_FILE=' + $toolchain, '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_INSTALL_PREFIX=' + $InstallPrefix)
  Write-Host "Trying generator: $($gen.Name)"
  Write-Host "$cmakeExe $($cmakeArgs -join ' ')"
  & $cmakeExe @cmakeArgs
  if ($LASTEXITCODE -eq 0) {
    $selectedGenerator = $gen.Name
    $buildDir = $candidateBuild
    break
  } else {
    Write-Warning "Generator $($gen.Name) failed with exit code $LASTEXITCODE"
  }
}

if (-not $selectedGenerator) { Fail "CMake configuration failed for all candidate generators" }

Write-Host "Selected generator: $selectedGenerator (build dir: $buildDir)"

Write-Host "Building and installing..."
if ($selectedGenerator -eq 'Ninja') {
  & $cmakeExe --build $buildDir --config Release --parallel 4
  if ($LASTEXITCODE -ne 0) { Fail "Build/install failed (Ninja)" }
} else {
  & $cmakeExe --build $buildDir --config Release --target INSTALL -- /m:4
  if ($LASTEXITCODE -ne 0) { Fail "Build/install failed (MSBuild)" }
}

# Ensure staging directory exists
if (-not (Test-Path $InstallPrefix)) { New-Item -ItemType Directory -Path $InstallPrefix -Force | Out-Null }

# Copy locale files into staging
if (Test-Path (Join-Path $RepoRoot 'locale')) {
    Write-Host "Copying locale/ into staging"
    $destLocale = Join-Path $InstallPrefix 'locale'
    Remove-Item -Recurse -Force $destLocale -ErrorAction SilentlyContinue
    Copy-Item -Path (Join-Path $RepoRoot 'locale') -Destination $destLocale -Recurse -Force
}

# Copy assets (icons, etc.) into staging
if (Test-Path (Join-Path $RepoRoot 'assets')) {
    Write-Host "Copying assets/ into staging"
    $destAssets = Join-Path $InstallPrefix 'assets'
    Remove-Item -Recurse -Force $destAssets -ErrorAction SilentlyContinue
    Copy-Item -Path (Join-Path $RepoRoot 'assets') -Destination $destAssets -Recurse -Force
}

# Attempt to locate the built executable and copy into staging/bin
$exeName = 'splinter3D.exe'
$found = Get-ChildItem -Path $buildDir -Recurse -Filter $exeName -ErrorAction SilentlyContinue | Select-Object -First 1
if ($found) {
    $binDir = Join-Path $InstallPrefix 'bin'
    if (-not (Test-Path $binDir)) { New-Item -ItemType Directory -Path $binDir | Out-Null }
    Copy-Item -Path $found.FullName -Destination (Join-Path $binDir $exeName) -Force
    Write-Host "Copied executable to staging/bin"
} else {
    Write-Warning "Executable $exeName not found in build tree; ensure CMake install rules place binaries into staging or adjust script to copy them explicitly."
}

if ($UseCPack) {
    Write-Host "Running CPack (requires NSIS or other packager)"
    Push-Location $buildDir
    & cpack -G NSIS || Write-Warning "CPack/NSIS failed or NSIS not installed"
    Pop-Location
}

# Create ZIP archive of staging tree
$zipPath = Join-Path $OutDir "release-package.zip"
if (Test-Path $zipPath) { Remove-Item $zipPath }
Write-Host "Creating ZIP: $zipPath"
Compress-Archive -Path (Join-Path $InstallPrefix '*') -DestinationPath $zipPath -Force || Fail "Failed to create ZIP"

# Compute SHA256
$shaFile = "$zipPath.sha256"
Write-Host "Computing SHA256 -> $shaFile"
[Byte[]]$data = Get-Content -Path $zipPath -Encoding Byte -ReadCount 0
$sha = [System.BitConverter]::ToString((New-Object Security.Cryptography.SHA256Managed).ComputeHash($data)).Replace('-', '').ToLowerInvariant()
$sha | Out-File -FilePath $shaFile -Encoding ascii

Write-Host "Packaging done. Artifacts:"
Write-Host " - ZIP: $zipPath"
Write-Host " - SHA256: $shaFile"
Write-Host "build-and-package.ps1 finished"

exit 0
