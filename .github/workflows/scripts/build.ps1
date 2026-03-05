param(
  [string]$ProjectRoot = (Get-Location).Path,
  [string]$BuildDirName = 'build',
  [string]$Triplet = 'x64-windows',
  [string]$Config = 'Release',
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

function Run-As-Administrator {
  $current = [Security.Principal.WindowsIdentity]::GetCurrent()
  $principal = New-Object Security.Principal.WindowsPrincipal($current)
  if ($env:GITHUB_ACTIONS -eq 'true' -or $env:CI -eq 'true') { Write-Host 'CI detected; skipping elevation'; return }
  if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole] 'Administrator')) {
    Write-Host 'Restarting script as administrator...'
    $args = @('-NoProfile','-ExecutionPolicy','Bypass')
    $args += '-File'; $args += $PSCommandPath
    if ($ProjectRoot) { $args += '-ProjectRoot'; $args += $ProjectRoot }
    if ($BuildDirName) { $args += '-BuildDirName'; $args += $BuildDirName }
    if ($Triplet) { $args += '-Triplet'; $args += $Triplet }
    if ($Config) { $args += '-Config'; $args += $Config }
    if ($OutDirName) { $args += '-OutDirName'; $args += $OutDirName }
    if ($DevMode) { $args += '-DevMode' }
    Start-Process -FilePath powershell -ArgumentList $args -Verb RunAs -Wait
    exit
  }
}

Write-Host "build.ps1 starting (ProjectRoot: $ProjectRoot; Config: $Config)"
try {
  $resolved = Resolve-Path -LiteralPath $ProjectRoot -ErrorAction Stop
  $ProjectRoot = [string]$resolved[0].ProviderPath
} catch {
  Fail "Project root not found or ambiguous: $ProjectRoot"
}

if (-not (Test-Path $ProjectRoot)) { Fail "Project root not found: $ProjectRoot" }

Run-As-Administrator

$buildDir = Join-Path $ProjectRoot $BuildDirName
if (Test-Path $buildDir) { Remove-Item -Recurse -Force $buildDir }
New-Item -ItemType Directory -Path $buildDir | Out-Null

# vcpkg toolchain: prefer ProjectRoot\vcpkg (install.ps1 bootstrapped there)
$vcpkgToolchain = Join-Path $ProjectRoot 'vcpkg\scripts\buildsystems\vcpkg.cmake'
if (-not (Test-Path $vcpkgToolchain)) {
  # fallback to sibling vcpkg
  $vcpkgToolchain = Join-Path (Join-Path (Split-Path $ProjectRoot -Parent) 'vcpkg') 'scripts\buildsystems\vcpkg.cmake'
}
if (Test-Path $vcpkgToolchain) { Write-Host "Using vcpkg toolchain: $vcpkgToolchain" } else { Write-Warning "vcpkg toolchain not found" }

# Try to locate vcpkg-installed tools (gettext/msgfmt) and prepend to PATH so CMake can find them
$toolPathsTried = @()
$foundToolPath = $null
$vcpkgRootCandidates = @()
try {
  $v1 = Join-Path -Path $ProjectRoot -ChildPath 'vcpkg'
  $v2 = Join-Path -Path (Split-Path -Path $ProjectRoot -Parent) -ChildPath 'vcpkg'
  $vcpkgRootCandidates = @($v1, $v2)
} catch {
  $vcpkgRootCandidates = @()
}
foreach ($root in $vcpkgRootCandidates) {
  if (-not $root) { continue }
  $cand = Join-Path $root "installed\$Triplet\tools\gettext\bin"
  $toolPathsTried += $cand
  if (Test-Path $cand) { $foundToolPath = $cand; break }
  # try packages path with possible ABI/hash
  $pkgPattern = Join-Path $root "packages\gettext_$Triplet*\tools\gettext\bin"
  $matches = Get-ChildItem -Path $pkgPattern -Directory -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
  if ($matches) { $foundToolPath = $matches[0]; break }
}
if ($foundToolPath) {
  Write-Host "Prepending vcpkg tools path to PATH: $foundToolPath"
  $env:PATH = "$foundToolPath;$env:PATH"
  $cmakeToolPaths = "-DCMAKE_PROGRAM_PATH=$foundToolPath"
} else {
  Write-Host "Did not find gettext/msgfmt in vcpkg locations; tried:`n  $($toolPathsTried -join "`n  ")"
  $cmakeToolPaths = ""
}

# Ensure msgfmt is available to CMake: if not in PATH, try to locate msgfmt.exe under vcpkg and prepend its folder
if (-not (Get-Command msgfmt -ErrorAction SilentlyContinue)) {
  Write-Host 'msgfmt not found on PATH. Searching vcpkg for msgfmt.exe...'
  $msgfmtCandidates = @()
  foreach ($root in $vcpkgRootCandidates) {
    if (-not (Test-Path $root)) { continue }
    try {
      $c = Get-ChildItem -Path (Join-Path $root '*') -Filter 'msgfmt.exe' -Recurse -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
      if ($c) { $msgfmtCandidates += $c }
    } catch { }
  }
  if ($msgfmtCandidates.Count -gt 0) {
    # Prefer the first candidate's folder
    $msgfmtExe = $msgfmtCandidates[0]
    $msgfmtDir = Split-Path -Path $msgfmtExe -Parent
    Write-Host "Found msgfmt at: $msgfmtExe -- prepending $msgfmtDir to PATH"
    $env:PATH = "$msgfmtDir;$env:PATH"
  } else {
    Write-Host 'No msgfmt.exe found under vcpkg directories.'
  }
} else {
  Write-Host "msgfmt found: $(Get-Command msgfmt | Select-Object -ExpandProperty Source)"
}

# If msgfmt still not on PATH, check common Chocolatey install locations and prepends their bin folders
if (-not (Get-Command msgfmt -ErrorAction SilentlyContinue)) {
  Write-Host 'msgfmt still not found; searching common Chocolatey install locations...'
  $chocoCandidates = @(
    'C:\Program Files\gettext-iconv\bin',
    'C:\Program Files\gettext\bin',
    'C:\ProgramData\chocolatey\lib\gettext*\tools\*',
    'C:\ProgramData\chocolatey\lib\gettext-iconv*\tools\*'
  )
  foreach ($pattern in $chocoCandidates) {
    try {
      $found = Get-ChildItem -Path $pattern -Filter 'msgfmt.exe' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
      if ($found) {
        $dir = Split-Path -Path $found.FullName -Parent
        $env:PATH = "$dir;$env:PATH"
        Write-Host "Found msgfmt at $($found.FullName); prepended $dir to PATH"
        break
      }
    } catch { }
  }
  if (-not (Get-Command msgfmt -ErrorAction SilentlyContinue)) { Write-Host 'msgfmt still not found after searching Chocolatey locations.' }
  else { Write-Host "msgfmt now: $(Get-Command msgfmt | Select-Object -ExpandProperty Source)" }
}

# Configure CMake
$cmakeArgs = @('-S', $ProjectRoot, '-B', $buildDir, '-G', 'Visual Studio 17 2022', '-A', 'x64', '-DWindows=ON')
if (Test-Path $vcpkgToolchain) { $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain" }
if ($cmakeToolPaths) { $cmakeArgs += $cmakeToolPaths }

Write-Host "Running CMake configure"
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
  Fail "CMake not found in PATH. Install CMake or add it to PATH: https://cmake.org/download/"
}
cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { Fail "CMake configure failed" }

Write-Host "Building ($Config)"
# Ensure MSBuild writes outputs into the build tree by overriding OutDir when
# using Visual Studio/MSBuild. This keeps artifacts under $buildDir\$Config\
$procCount = [Environment]::ProcessorCount
$outDirMsbuild = Join-Path $buildDir $Config
# ensure trailing backslash for MSBuild OutDir
if ($outDirMsbuild[-1] -ne '\') { $outDirMsbuild = "$outDirMsbuild\" }
$msbuildArgs = @("/m:$procCount", "/p:OutDir=$outDirMsbuild")
cmake --build $buildDir --config $Config -- $msbuildArgs
if ($LASTEXITCODE -ne 0) { Fail "Build failed" }

# Ensure the splinter3D target(s) are explicitly built (try both possible target names)
$targetsToTry = @('splinter3D','splinter3D-app')
foreach ($t in $targetsToTry) {
  Write-Host "Attempting to build target: $t"
  cmake --build $buildDir --config $Config --target $t -- $msbuildArgs 2>$null
}

# Prepare staging area
$staging = Join-Path $ProjectRoot 'staging'
if (Test-Path $staging) { Remove-Item -Recurse -Force $staging }
New-Item -ItemType Directory -Path $staging | Out-Null

# Find executables produced by the build and copy them into staging/bin
$binDir = Join-Path $staging 'bin'
New-Item -ItemType Directory -Path $binDir | Out-Null

# Heuristic: look for any .exe in build tree whose name contains 'splinter3D' (covers splinter3D.exe and splinter3D-app.exe)
Write-Host 'Searching build output for splinter3D executable...'

# Primary search: executables under the build directory
$foundExe = Get-ChildItem -Path $buildDir -Recurse -Filter '*.exe' -ErrorAction SilentlyContinue |
  Where-Object { $_.Name -like '*splinter3D*.exe' -or $_.Name -eq 'splinter3D.exe' } |
  Sort-Object LastWriteTime -Descending | Select-Object -First 1

# If not found, also check common MSVC output locations (project-root\Release, project-root\$Config, build\$Config)
if (-not $foundExe) {
  $extraCandidates = @(
    (Join-Path $ProjectRoot $Config),
    (Join-Path $ProjectRoot 'Release'),
    (Join-Path $buildDir $Config),
    (Join-Path $buildDir 'Release'),
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

  # Also check for a top-level Release exe placed at ProjectRoot\Release\splinter3D.exe
  if (-not $foundExe) {
    $topExe = Join-Path (Join-Path $ProjectRoot 'Release') 'splinter3D.exe'
    if (Test-Path $topExe) { $foundExe = Get-Item $topExe }
  }
}

if ($foundExe) {
  $destPath = Join-Path $binDir 'splinter3D.exe'
  Copy-Item -Path $foundExe.FullName -Destination $destPath -Force
  Write-Host "Copied $($foundExe.FullName) to staging/bin as splinter3D.exe"
} else {
  Write-Warning 'No splinter3D executable found in build tree; build may have failed or target name differs.'
}

# Copy any runtime DLLs next to the exe into staging/bin
$dlls = Get-ChildItem -Path $buildDir -Recurse -Filter '*.dll' -ErrorAction SilentlyContinue
foreach ($d in $dlls) {
  Copy-Item -Path $d.FullName -Destination (Join-Path $binDir $d.Name) -Force
}

# Copy locale and assets
if (Test-Path (Join-Path $ProjectRoot 'locale')) { Copy-Item -Path (Join-Path $ProjectRoot 'locale') -Destination (Join-Path $staging 'locale') -Recurse -Force }
if (Test-Path (Join-Path $ProjectRoot 'assets')) { Copy-Item -Path (Join-Path $ProjectRoot 'assets') -Destination (Join-Path $staging 'assets') -Recurse -Force }

# Copy other useful files
foreach ($f in @('README.md','LICENSE*')) {
  $src = Join-Path $ProjectRoot $f
  if (Get-ChildItem -Path $src -ErrorAction SilentlyContinue) { Copy-Item -Path $src -Destination $staging -Recurse -Force }
}

# Create output dir and package
$outDir = Join-Path $ProjectRoot $OutDirName
if (Test-Path $outDir) { Remove-Item -Recurse -Force $outDir }
New-Item -ItemType Directory -Path $outDir | Out-Null

$zipPath = Join-Path $outDir 'release-package.zip'
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
Write-Host "SHA256: $($sha.Hash) -> $shaFile"
Write-Host "build.ps1 finished. Staging: $staging; Output: $outDir"

# If a top-level Release folder was left behind and it's empty, remove it to keep
# the repository clean (some MSBuild setups create this folder transiently).
$topLevelRelease = Join-Path $ProjectRoot 'Release'
if (Test-Path $topLevelRelease) {
  try {
    $entries = Get-ChildItem -LiteralPath $topLevelRelease -Force -ErrorAction SilentlyContinue
    if (-not $entries -or $entries.Count -eq 0) {
      Remove-Item -Recurse -Force $topLevelRelease -ErrorAction SilentlyContinue
      if (-not (Test-Path $topLevelRelease)) { Write-Host "Removed empty top-level Release folder: $topLevelRelease" }
    }
  } catch {
    Write-Warning "Failed to inspect or remove top-level Release folder: $_"
  }
}

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

if ($DevMode) {
  Write-Host ''
  Write-Host 'DevMode active. Press Enter to close this window.'
  try { Read-Host | Out-Null } catch { }
}
