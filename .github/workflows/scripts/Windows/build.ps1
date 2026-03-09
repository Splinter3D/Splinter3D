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
# Also consider a vcpkg_installed under the build directory (CI sometimes places vcpkg output there)
$buildVcpkg = Join-Path $buildDir 'vcpkg_installed'
if (Test-Path $buildVcpkg) { $vcpkgRootCandidates += $buildVcpkg }
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

# Ensure msgfmt is available to CMake: if not in PATH, try a few places and prepend the folder when found
if (-not (Get-Command msgfmt -ErrorAction SilentlyContinue)) {
  Write-Host 'msgfmt not found on PATH. Searching vcpkg roots for msgfmt.exe...'
  $msgfmtCandidates = @()
  foreach ($root in $vcpkgRootCandidates) {
    if (-not (Test-Path $root)) { continue }
    try {
      # Search recursively under candidate roots (covers installed/<triplet>/tools and packages/*/tools)
      $c = Get-ChildItem -Path (Join-Path $root '*') -Filter 'msgfmt.exe' -Recurse -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
      if ($c) { $msgfmtCandidates += $c }
    } catch { }
  }
  if ($msgfmtCandidates.Count -gt 0) {
    $msgfmtExe = $msgfmtCandidates[0]
    $msgfmtDir = Split-Path -Path $msgfmtExe -Parent
    Write-Host "Found msgfmt at: $msgfmtExe -- prepending $msgfmtDir to PATH"
    $env:PATH = "$msgfmtDir;$env:PATH"
  } else {
    Write-Host 'No msgfmt.exe found under vcpkg roots. Trying common Chocolatey locations...'
    # Try common Chocolatey locations
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

    # Final fallback: search the build tree for any msgfmt.exe (useful when vcpkg installs into build/vcpkg_installed)
    if (-not (Get-Command msgfmt -ErrorAction SilentlyContinue)) {
      try {
        Write-Host 'Final scan: searching build directory for msgfmt.exe...'
        $found = Get-ChildItem -Path $buildDir -Filter 'msgfmt.exe' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) {
          $dir = Split-Path -Path $found.FullName -Parent
          $env:PATH = "$dir;$env:PATH"
          Write-Host "Found msgfmt at $($found.FullName); prepended $dir to PATH"
        } else {
          Write-Host 'Final scan: msgfmt.exe not found in build tree.'
        }
      } catch { Write-Host "Final scan error: $_" }
    }
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
$cmakeArgs = @('-S', $ProjectRoot, '-B', $buildDir, '-G', 'Visual Studio 17 2022', '-A', 'x64')
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

# Clean up any stray Release folder left behind by some MSBuild configurations

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

Write-Host "build.ps1 finished. Now calling package-windows.ps1..."

# Call the packaging script
$packageScript = Join-Path (Split-Path -Parent $PSCommandPath) 'package-windows.ps1'
if (-not (Test-Path $packageScript)) {
  Fail "Packaging script not found: $packageScript"
}

# Ensure ProjectRoot is valid before invoking packaging
if (-not $ProjectRoot -or $ProjectRoot -match '^-') { Fail "Invalid ProjectRoot: $ProjectRoot" }

# Call the packaging script using explicit named parameters to avoid
# ambiguity when passing arguments through arrays or quoting layers.
try {
  & $packageScript -ProjectRoot $ProjectRoot -BuildDirName $BuildDirName -Triplet $Triplet -OutDirName $OutDirName
  $packageExitCode = $LASTEXITCODE
  if ($packageExitCode -ne 0) {
    Write-Error "Packaging script exited with code: $packageExitCode"
    exit $packageExitCode
  }
} catch {
  Fail "Packaging script failed: $_"
}

if ($DevMode) {
  Write-Host ''
  Write-Host 'DevMode active. Press Enter to close this window.'
  try { Read-Host | Out-Null } catch { }
}
