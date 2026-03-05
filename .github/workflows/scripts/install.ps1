param(
  [string]$ProjectRoot = "",
  [string]$Triplet = "x64-windows",
  [switch]$SetupMSVC = $true,
  [switch]$DevMode = $false
)

function Fail([string]$msg) {
  Write-Error $msg
  if (Get-Command -Name Write-Log -ErrorAction SilentlyContinue) { Write-Log 'ERROR' $msg }
  exit 1
}

# Normalize ProjectRoot
if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
  if ($env:GITHUB_WORKSPACE) {
    $ProjectRoot = $env:GITHUB_WORKSPACE
  } else {
    # default to repository root relative to this script when possible
    try {
      $repoRootGuess = Join-Path $PSScriptRoot '..\..\..'
      $repoRootResolved = Resolve-Path -LiteralPath $repoRootGuess -ErrorAction Stop
      $ProjectRoot = $repoRootResolved[0].ProviderPath
    } catch {
      # fallback to current location
      $ProjectRoot = (Get-Location).Path
    }
  }
}
try {
  $ProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot -ErrorAction Stop).Path
} catch {
  Log-Warn "Project root path could not be resolved: $ProjectRoot. Falling back to current directory."
  $ProjectRoot = (Get-Location).Path
}
Write-Host "install.ps1 starting (ProjectRoot: $ProjectRoot)"

# Setup logging directory and helpers
$LogDir = Join-Path $ProjectRoot '.install_logs'
if (-not (Test-Path $LogDir)) { New-Item -ItemType Directory -Force -Path $LogDir | Out-Null }
$LogFile = Join-Path $LogDir ('install_' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '.log')
function Write-Log([string]$level, [string]$msg) {
  $line = ("[{0}] {1}: {2}" -f (Get-Date -Format 's'), $level, $msg)
  Write-Host $line
  try { Add-Content -Path $LogFile -Value $line -Encoding utf8 } catch {}
}
function Log-Milestone([string]$msg) { Write-Log 'MILESTONE' $msg }
function Log-Action([string]$msg) { Write-Log 'ACTION' $msg }
function Log-Info([string]$msg) { Write-Log 'INFO' $msg }
function Log-Warn([string]$msg) { Write-Log 'WARN' $msg }
function Log-ErrorMsg([string]$msg) { Write-Log 'ERROR' $msg }

Log-Milestone ("Script started. ProjectRoot=$ProjectRoot; Triplet=$Triplet; SetupMSVC=$SetupMSVC; DevMode=$DevMode")

function Import-VcvarsEnvironment([string]$Arch = 'x64') {
  Log-Action 'Locating vswhere...'
  $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
  if (-not (Test-Path $vswhere)) {
    $cmd = Get-Command vswhere -ErrorAction SilentlyContinue
    if ($cmd) { $vswhere = $cmd.Source }
  }
  if (-not (Test-Path $vswhere)) { Log-Warn 'vswhere not found; skipping MSVC import'; return }

  $instPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
  if (-not $instPath) { $instPath = & $vswhere -latest -products * -property installationPath 2>$null }
  if (-not $instPath) { Log-Warn 'No Visual Studio installation found via vswhere'; return }

  $vcvars = Join-Path $instPath 'VC\Auxiliary\Build\vcvarsall.bat'
  if (-not (Test-Path $vcvars)) { Log-Warn "vcvarsall.bat not found at $vcvars"; return }

  Log-Action "Importing MSVC environment from $vcvars ($Arch)"
  $envOutput = cmd /c "call `"$vcvars`" $Arch && set" 2>$null
  foreach ($line in $envOutput) {
    if ($line -match '^(.*?)=(.*)$') {
      $key = $matches[1]
      $val = $matches[2]
      try { [System.Environment]::SetEnvironmentVariable($key, $val, 'Process') } catch { }
    }
  }
  Log-Milestone 'MSVC environment imported.'
}

function Run-As-Administrator {
  $current = [Security.Principal.WindowsIdentity]::GetCurrent()
  $principal = New-Object Security.Principal.WindowsPrincipal($current)
  if ($env:GITHUB_ACTIONS -eq 'true' -or $env:CI -eq 'true') { Write-Host 'CI detected; skipping elevation'; return }
  if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole] 'Administrator')) {
    Log-Action 'Relaunching elevated to acquire Administrator privileges...'
    $args = @('-NoProfile','-ExecutionPolicy','Bypass','-File', $PSCommandPath)
    if ($ProjectRoot) { $args += '-ProjectRoot'; $args += $ProjectRoot }
    if ($Triplet) { $args += '-Triplet'; $args += $Triplet }
    if ($SetupMSVC) { $args += '-SetupMSVC' }
    if ($DevMode) { $args += '-DevMode' }
    try {
      Start-Process -FilePath powershell -ArgumentList $args -Verb RunAs -WorkingDirectory $ProjectRoot -Wait
      Log-Info 'Elevated process launched; exiting current process.'
      exit
    } catch {
      Log-ErrorMsg ("Failed to relaunch elevated process: {0}" -f $_)
      Fail 'Elevation canceled or failed; please run the script from an elevated PowerShell manually.'
    }
  }
}

## Begin execution: bootstrap vcpkg into project root and install deps (classic mode)
Run-As-Administrator

# Clone/bootstrap vcpkg into the project root (not the parent folder)
$vcRoot = Join-Path $ProjectRoot 'vcpkg'
if (-not (Test-Path $vcRoot)) {
  Log-Action "Cloning vcpkg into $vcRoot"
  try { git clone https://github.com/microsoft/vcpkg.git $vcRoot; Log-Milestone 'vcpkg clone completed' } catch { Log-ErrorMsg ("git clone failed: {0}" -f $_); Fail 'git clone failed' }
}
Push-Location $vcRoot
if (Test-Path .\bootstrap-vcpkg.bat) {
  Log-Action 'Running bootstrap-vcpkg.bat'
  try { & .\bootstrap-vcpkg.bat; Log-Milestone 'bootstrap-vcpkg completed' } catch { Log-ErrorMsg ("bootstrap-vcpkg failed: {0}" -f $_); }
} else {
  Log-Warn 'bootstrap-vcpkg.bat missing'
}
$vcpkgExe = Join-Path $vcRoot 'vcpkg.exe'
if (Test-Path $vcpkgExe) {
  Log-Action 'Registering vcpkg (integrate install)'
  try { & $vcpkgExe integrate install; Log-Milestone 'vcpkg integrated (user)'} catch { Log-ErrorMsg ("vcpkg integrate failed: {0}" -f $_) }
} else {
  Log-ErrorMsg 'vcpkg.exe missing after bootstrap; aborting'
  Pop-Location
  Fail 'vcpkg.exe missing after bootstrap'
}
$LogInfoMsg = $null
## Import MSVC environment before attempting to install via vcpkg (required for detect_compiler)
if ($SetupMSVC) {
  Log-Action 'Importing MSVC environment (pre-install)'
  try { Import-VcvarsEnvironment -Arch 'x64'; Log-Milestone 'MSVC environment imported (pre-install)' } catch { Log-Warn ("Import-VcvarsEnvironment failed: {0}" -f $_) }
}

## Install dependencies using manifest mode when a vcpkg.json manifest exists.
$jsonPath = Join-Path $ProjectRoot 'vcpkg.json'
## Ensure required external tools (cmake, msgfmt) are present or try to install via winget
function Ensure-Command-With-Winget([string]$cmd, [string]$wingetId) {
  try {
    if (Get-Command $cmd -ErrorAction SilentlyContinue) { Log-Info "$cmd present"; return $true }
  } catch { }
  if (-not (Get-Command winget -ErrorAction SilentlyContinue)) { Log-Warn "winget not available; cannot install $cmd"; return $false }
  Log-Action "Attempting to install $cmd via winget ($wingetId)"
  try {
    & winget install --id $wingetId --accept-package-agreements --accept-source-agreements -e
    if ($LASTEXITCODE -eq 0 -and (Get-Command $cmd -ErrorAction SilentlyContinue)) { Log-Milestone "$cmd installed via winget"; return $true }
    Log-Warn "$cmd installation via winget did not succeed or command still not found"
    return $false
  } catch {
    Log-Warn ("winget install threw an exception: {0}" -f $_)
    return $false
  }
}

# Ensure Chocolatey is available (install it if missing)
function Ensure-Chocolatey() {
  if (Get-Command choco -ErrorAction SilentlyContinue) { Log-Info 'choco present'; return $true }
  Log-Action 'Chocolatey not found; attempting automated install (this requires elevation)'
  try {
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    $script = (New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1')
    if (-not $script) { Log-Warn 'Failed to download Chocolatey install script'; return $false }
    Invoke-Expression $script
    Start-Sleep -Seconds 2
    # Reload PATH from system and user environment so the current session can see newly-installed programs
    try {
      $machinePath = [System.Environment]::GetEnvironmentVariable('Path','Machine')
      $userPath = [System.Environment]::GetEnvironmentVariable('Path','User')
      $combined = ($machinePath + ';' + $userPath).Trim(';')
      if ($combined) {
        $env:Path = $combined
        Log-Info 'Session PATH reloaded from system/user environment'
      }
    } catch {
      Log-Warn ("Failed to reload session PATH: {0}" -f $_)
    }
    if (Get-Command choco -ErrorAction SilentlyContinue) { Log-Milestone 'Chocolatey installed'; return $true }
    Log-Warn 'Chocolatey install did not make choco available in this session'
    return $false
  } catch {
    Log-Warn ("Chocolatey install failed: {0}" -f $_)
    return $false
  }
}

# Install a command using Chocolatey
function Ensure-Command-With-Choco([string]$cmd, [string]$chocoPkg) {
  try { if (Get-Command $cmd -ErrorAction SilentlyContinue) { Log-Info "$cmd present"; return $true } } catch { }
  if (-not (Get-Command choco -ErrorAction SilentlyContinue)) { Log-Warn 'choco not available; cannot install with Chocolatey'; return $false }
  Log-Action "Attempting to install $chocoPkg via choco"
  try {
    & choco install $chocoPkg -y --no-progress
    Start-Sleep -Seconds 2
    if (Get-Command $cmd -ErrorAction SilentlyContinue) { Log-Milestone "$cmd installed via choco"; return $true }
    Log-Warn "$cmd installation via choco did not succeed or command still not found"
    return $false
  } catch {
    Log-Warn ("choco install threw an exception: {0}" -f $_)
    return $false
  }
}

## Try to ensure cmake and msgfmt are present (winget-only)
## Try to ensure cmake and msgfmt are present: prefer Chocolatey and use .choco-dependencies if provided,
## otherwise fall back to winget for missing tools.
$chocoAvailable = Ensure-Chocolatey
$chocoDepsFile = Join-Path $ProjectRoot '.choco-dependencies'
if ($chocoAvailable -and (Test-Path $chocoDepsFile)) {
  Log-Action ("Found .choco-dependencies at {0}; installing listed packages" -f $chocoDepsFile)
  try {
    $lines = Get-Content -Path $chocoDepsFile -ErrorAction Stop
    foreach ($line in $lines) {
      $pkg = $line.Trim()
      if ([string]::IsNullOrWhiteSpace($pkg)) { continue }
      if ($pkg.StartsWith('#')) { continue }
      if ($pkg -match '^(.*?)#') { $pkg = $matches[1].Trim() }
      if (-not $pkg) { continue }
      Log-Action ("Installing Chocolatey package: {0}" -f $pkg)
      try {
        & choco install $pkg -y --no-progress
        if ($LASTEXITCODE -eq 0) { Log-Milestone ("choco: {0} installed" -f $pkg) } else { Log-Warn ("choco install {0} exited with {1}" -f $pkg, $LASTEXITCODE) }
      } catch {
        Log-Warn ("Exception when installing {0} via choco: {1}" -f $pkg, $_)
      }
    }
  } catch {
    Log-Warn ("Failed to read .choco-dependencies: {0}" -f $_)
  }
}

# Verify tools are available; if missing, attempt winget as fallback
if (Get-Command cmake -ErrorAction SilentlyContinue) { $cmakeOk = $true } else { $cmakeOk = Ensure-Command-With-Winget 'cmake' 'Kitware.CMake' }
if (Get-Command msgfmt -ErrorAction SilentlyContinue) { $msgfmtOk = $true } else { $msgfmtOk = Ensure-Command-With-Winget 'msgfmt' 'GnuWin32.Gettext' }
if (-not $cmakeOk) { Log-Warn 'CMake not available; CMake configure may fail.' }
if (-not $msgfmtOk) { Log-Warn 'msgfmt not available; gettext tools may be missing for configure.' }
if (Test-Path $jsonPath) {
  Log-Action ("Found manifest at {0} - running manifest install (triplet: {1})" -f $jsonPath, $Triplet)
  try {
    Log-Action 'Unsetting VCPKG_ROOT for this process and invoking vcpkg with explicit --vcpkg-root'
    try { [System.Environment]::SetEnvironmentVariable('VCPKG_ROOT',$null,'Process'); Log-Info 'VCPKG_ROOT unset for process' } catch { Log-Warn "Failed to unset VCPKG_ROOT: $_" }
    & $vcpkgExe install --vcpkg-root $vcRoot --triplet $Triplet
    if ($LASTEXITCODE -eq 0) { Log-Milestone 'vcpkg manifest install completed successfully' } else { Log-ErrorMsg ("vcpkg manifest install failed (exit {0})" -f $LASTEXITCODE) }
  } catch {
    Log-ErrorMsg ("vcpkg manifest install threw an exception: {0}" -f $_)
  }
} else {
  Log-Warn ("vcpkg.json not found at {0}; no dependencies installed." -f $jsonPath)
}

if ($SetupMSVC) { Import-VcvarsEnvironment -Arch 'x64' }
Pop-Location

Write-Host 'vcpkg install complete; run build.ps1 to configure and build'
if ($DevMode) {
  Write-Host ''
  Write-Host 'DevMode active. Press Enter to close this window.'
  try { Read-Host | Out-Null } catch { }
}
