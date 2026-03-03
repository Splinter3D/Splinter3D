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

# Ensure vcpkg exists and is bootstrapped
if (-not (Test-Path $VcpkgDir)) {
    Write-Host "Cloning vcpkg into $VcpkgDir..."
    git clone https://github.com/microsoft/vcpkg.git $VcpkgDir || Fail "Failed to clone vcpkg"
}

Push-Location $VcpkgDir
if (-not (Test-Path "$VcpkgDir\vcpkg.exe")) {
    Write-Host "Bootstrapping vcpkg..."
    & .\bootstrap-vcpkg.bat || Fail "vcpkg bootstrap failed"
}

Write-Host "Installing vcpkg manifest dependencies (if present)..."
try {
    & .\vcpkg.exe install --triplet $Triplet --manifest
} catch {
    Write-Warning "vcpkg manifest install failed or no manifest; continuing. Error: $_"
}
Pop-Location

# Prepare build directory
$buildDir = Join-Path $RepoRoot "build"
if (Test-Path $buildDir) { Remove-Item -Recurse -Force $buildDir }
New-Item -ItemType Directory -Path $buildDir | Out-Null

$toolchain = Join-Path $VcpkgDir "scripts\buildsystems\vcpkg.cmake"
if (-not (Test-Path $toolchain)) { Fail "vcpkg toolchain not found at $toolchain" }

Write-Host "Configuring CMake..."
$cmakeExe = "cmake"
$cmakeArgs = @(
    "-S", "`"$RepoRoot`"",
    "-B", "`"$buildDir`"",
    "-G", '"Visual Studio 17 2022"',
    "-A", "x64",
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_INSTALL_PREFIX=$InstallPrefix"
)

$cmakeCmd = "$cmakeExe $($cmakeArgs -join ' ')"
Write-Host $cmakeCmd
& $cmakeExe $cmakeArgs || Fail "CMake configuration failed"

Write-Host "Building and installing..."
& $cmakeExe --build $buildDir --config Release --target INSTALL -- -m:4 || Fail "Build/install failed"

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
