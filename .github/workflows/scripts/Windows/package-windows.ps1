param(
  [string]$ProjectRoot = (Get-Location).Path,
  [string]$BuildDirName = 'build',
  [string]$Triplet = 'x64-windows',
  [string]$OutDirName = 'output',
  [switch]$DevMode = $false
)

function Fail($msg) {
  Write-Error $msg
  if ($DevMode) {
    Write-Host ''
    Write-Host 'DevMode active. An error occurred. Press Enter to exit.'
    try { Read-Host | Out-Null } catch { }
  }
  exit 1
}

Write-Host "package-windows.ps1 starting (ProjectRoot: $ProjectRoot)"

try {
  $resolved = Resolve-Path -LiteralPath $ProjectRoot -ErrorAction Stop
  $ProjectRoot = [string]$resolved[0].ProviderPath
} catch {
  Fail "Project root not found or ambiguous: $ProjectRoot"
}

if (-not (Test-Path $ProjectRoot)) { Fail "Project root not found: $ProjectRoot" }

$buildDir = Join-Path $ProjectRoot $BuildDirName
$staging = Join-Path $ProjectRoot 'staging'
$outDir = Join-Path $ProjectRoot $OutDirName

# ============================================================================
# Prepare staging area
# ============================================================================

if (Test-Path $staging) { Remove-Item -Recurse -Force $staging }
New-Item -ItemType Directory -Path $staging | Out-Null

# Find executables produced by the build and copy them into staging/bin
$binDir = Join-Path $staging 'bin'
New-Item -ItemType Directory -Path $binDir | Out-Null

Write-Host 'Searching build output for splinter3D executable...'

# Primary search: executables under the build directory
$foundExe = Get-ChildItem -Path $buildDir -Recurse -Filter '*.exe' -ErrorAction SilentlyContinue |
  Where-Object { $_.Name -like '*splinter3D*.exe' -or $_.Name -eq 'splinter3D.exe' } |
  Sort-Object LastWriteTime -Descending | Select-Object -First 1

# If not found, also check common MSVC output locations
if (-not $foundExe) {
  $extraCandidates = @(
    $ProjectRoot,
    (Join-Path $ProjectRoot 'Release'),
    (Join-Path $ProjectRoot 'Debug'),
    (Join-Path $buildDir 'Release'),
    (Join-Path $buildDir 'Debug'),
    (Join-Path $ProjectRoot 'bin'),
    (Join-Path $buildDir 'bin')
  ) | Where-Object { $_ -and (Test-Path $_) } | Get-Unique

  foreach ($dir in $extraCandidates) {
    try {
      $cand = Get-ChildItem -Path $dir -Recurse -Filter '*.exe' -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like '*splinter3D*.exe' -or $_.Name -eq 'splinter3D.exe' } |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
      if ($cand) { $foundExe = $cand; break }
    } catch { }
  }
}

if ($foundExe) {
  $destPath = Join-Path $binDir 'splinter3D.exe'
  Copy-Item -Path $foundExe.FullName -Destination $destPath -Force
  Write-Host "Copied $($foundExe.FullName) to staging/bin as splinter3D.exe"
} else {
  Fail 'No splinter3D executable found in build tree'
}

# Copy any runtime DLLs next to the exe into staging/bin
$dlls = Get-ChildItem -Path $buildDir -Recurse -Filter '*.dll' -ErrorAction SilentlyContinue
foreach ($d in $dlls) {
  Copy-Item -Path $d.FullName -Destination (Join-Path $binDir $d.Name) -Force
}

# Copy locale and assets
if (Test-Path (Join-Path $ProjectRoot 'locale')) { Copy-Item -Path (Join-Path $ProjectRoot 'locale') -Destination (Join-Path $staging 'locale') -Recurse -Force }
if (Test-Path (Join-Path $ProjectRoot 'assets')) { Copy-Item -Path (Join-Path $ProjectRoot 'assets') -Destination (Join-Path $staging 'assets') -Recurse -Force }

# Copy documentation
foreach ($f in @('README.md','LICENSE*')) {
  $src = Join-Path $ProjectRoot $f
  if (Get-ChildItem -Path $src -ErrorAction SilentlyContinue) { Copy-Item -Path $src -Destination $staging -Recurse -Force }
}

# ============================================================================
# Auto-detect Version from scm
# ============================================================================

$version = git describe --tags --abbrev=0 2>$null
if (-not $version) {
  Fail "Cannot auto-detect version: no git tags found. Please ensure that the repository has at least one tag."
} else {
  Write-Host "Auto-detected version: $version"
}

# Extract architecture from triplet (e.g., x64-windows -> x64)
$arch = $Triplet -split '-' | Select-Object -First 1

# ============================================================================
# Create output dir and package
# ============================================================================

if (Test-Path $outDir) { Remove-Item -Recurse -Force $outDir }
New-Item -ItemType Directory -Path $outDir | Out-Null

$zipName = "splinter3D-${version}-windows-${arch}.zip"
$zipPath = Join-Path $outDir $zipName
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }

Write-Host "Creating ZIP: $zipPath"
try {
  # Remove source PO files from staging so the distributable contains only compiled .mo files
  $poFiles = Get-ChildItem -Path $staging -Recurse -Filter '*.po' -ErrorAction SilentlyContinue
  if ($poFiles) {
    Write-Host "Removing source PO files from staging..."
    foreach ($f in $poFiles) { Remove-Item -Path $f.FullName -Force -ErrorAction SilentlyContinue }
  }

  Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $zipPath -Force
} catch {
  Fail "Failed to create ZIP: $_"
}

# Write SHA256
$sha = Get-FileHash -Path $zipPath -Algorithm SHA256
$shaFile = "$zipPath.sha256"
$sha.Hash | Out-File -FilePath $shaFile -Encoding ascii

Write-Host "Packaging complete: $zipPath"
Write-Host "SHA256: $($sha.Hash)"

# Remove staging directory now that the ZIP is created, unless DevMode is active
if (-not $DevMode) {
  try {
    Remove-Item -Recurse -Force $staging -ErrorAction SilentlyContinue
    Write-Host "Removed staging directory: $staging"
  } catch {
    Write-Warning "Failed to remove staging directory: $_"
  }
} else {
  Write-Host 'DevMode active; keeping staging for inspection.'
}

Write-Host "package-windows.ps1 finished. Output: $outDir"
exit 0
