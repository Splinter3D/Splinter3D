param(
  [string]$ProjectRoot = (Get-Location).Path,
  [string]$BuildDirName = 'build',
  [string]$Triplet = 'x64-windows'
)

function Fail($msg) { Write-Error $msg; exit 1 }

Write-Host "build.ps1 starting (ProjectRoot: $ProjectRoot)"
$ProjectRoot = (Resolve-Path $ProjectRoot).Path
if (-not (Test-Path $ProjectRoot)) { Fail "Project root not found: $ProjectRoot" }

$buildDir = Join-Path $ProjectRoot $BuildDirName
if (Test-Path $buildDir) { Remove-Item -Recurse -Force $buildDir }
New-Item -ItemType Directory -Path $buildDir | Out-Null

# Expect vcpkg to be a sibling folder to the repo root
$vcpkgToolchain = Join-Path (Join-Path (Split-Path $ProjectRoot -Parent) 'vcpkg') 'scripts\buildsystems\vcpkg.cmake'
if (-not (Test-Path $vcpkgToolchain)) {
  Write-Warning "vcpkg toolchain not found at $vcpkgToolchain";
} else {
  Write-Host "Using vcpkg toolchain: $vcpkgToolchain"
}

# Try to locate vcpkg-installed tools (e.g. gettext's msgfmt) and prepend to PATH so
# CMake can find programs during configure. This covers both manifest-mode installs
# under the build directory and classic installs under the vcpkg repo.
$toolPathsTried = @()
$foundToolPath = $null
$candidates = @(
  Join-Path $buildDir "vcpkg_installed\$Triplet\tools\gettext\bin",
  Join-Path $ProjectRoot "vcpkg_installed\$Triplet\tools\gettext\bin",
  Join-Path (Join-Path (Split-Path $ProjectRoot -Parent) 'vcpkg') "installed\$Triplet\tools\gettext\bin",
  Join-Path (Join-Path (Split-Path $ProjectRoot -Parent) 'vcpkg') "vcpkg_installed\$Triplet\tools\gettext\bin",
  Join-Path (Join-Path (Split-Path $ProjectRoot -Parent) 'vcpkg') "tools\gettext\bin"
)
foreach ($p in $candidates) {
  $toolPathsTried += $p
  if (Test-Path $p) { $foundToolPath = $p; break }
}
if ($foundToolPath) {
  Write-Host "Prepending vcpkg tools path to PATH: $foundToolPath"
  $env:PATH = "$foundToolPath;$env:PATH"
  # Also instruct CMake to look for programs in this path
  $cmakeToolPaths = "-DCMAKE_PROGRAM_PATH=$foundToolPath"
} else {
  Write-Host "Did not find gettext tools in common vcpkg locations; tried:`n  $($toolPathsTried -join "`n  ")"
  $cmakeToolPaths = ""
}

# Configure
$cmakeArgs = @('-S', $ProjectRoot, '-B', $buildDir, '-G', 'Visual Studio 17 2022', '-A', 'x64')
if (Test-Path $vcpkgToolchain) { $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain" }

Write-Host "Running CMake configure"
cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { Fail "CMake configure failed" }

Write-Host "Building (Release)"
cmake --build $buildDir --config Release -- /m:$(Get-Process -Id $PID -ErrorAction SilentlyContinue | ForEach-Object { [Environment]::ProcessorCount })
if ($LASTEXITCODE -ne 0) { Fail "Build failed" }

# Attempt to copy built executable(s) into a staging area
$staging = Join-Path $ProjectRoot 'staging'
if (Test-Path $staging) { Remove-Item -Recurse -Force $staging }
New-Item -ItemType Directory -Path $staging | Out-Null

$exeName = 'splinter3D.exe'
$found = Get-ChildItem -Path $buildDir -Recurse -Filter $exeName -ErrorAction SilentlyContinue | Select-Object -First 1
if ($found) {
  $binDir = Join-Path $staging 'bin'
  New-Item -ItemType Directory -Path $binDir | Out-Null
  Copy-Item -Path $found.FullName -Destination (Join-Path $binDir $exeName) -Force
  Write-Host "Copied $exeName to staging/bin"
} else {
  Write-Warning "$exeName not found in build tree"
}

# Copy locale and assets
if (Test-Path (Join-Path $ProjectRoot 'locale')) { Copy-Item -Path (Join-Path $ProjectRoot 'locale') -Destination (Join-Path $staging 'locale') -Recurse -Force }
if (Test-Path (Join-Path $ProjectRoot 'assets')) { Copy-Item -Path (Join-Path $ProjectRoot 'assets') -Destination (Join-Path $staging 'assets') -Recurse -Force }

Write-Host "build.ps1 finished. Staging: $staging"
