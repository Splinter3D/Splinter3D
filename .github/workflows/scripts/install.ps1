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
  if ($env:GITHUB_WORKSPACE) { $ProjectRoot = $env:GITHUB_WORKSPACE } else { $ProjectRoot = (Get-Location).Path }
}
try { $ProjectRoot = (Resolve-Path $ProjectRoot).Path } catch { Fail "Project root not found: $ProjectRoot" }
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
    Start-Process -FilePath powershell -ArgumentList $args -Verb RunAs -Wait
    Log-Info 'Elevated process launched; exiting current process.'
    exit
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
