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
  Write-Host "vcpkg.exe not found"
  Write-Host "Bootstrapping from source"
  try {
      & .\bootstrap-vcpkg.bat
  } catch {
      Write-Error "vcpkg bootstrap failed: $_"
      Pop-Location
      Fail "vcpkg bootstrap failed"
  }
}

# Integrate and ensure vcpkg works (only if executable present and valid)
if (Test-Path $vcpkgExe) {
  try {
    # Double-check header before running using .NET to avoid PowerShell encoding compatibility issues
    $bytes = [System.IO.File]::ReadAllBytes($vcpkgExe)
    if ($bytes.Length -ge 2 -and $bytes[0] -eq 0x4d -and $bytes[1] -eq 0x5a) {
      # Suppress non-terminating warnings from PowerShell parameter binding inside the integrate helper
      $oldWarning = $WarningPreference
      try {
        $WarningPreference = 'SilentlyContinue'
        & $vcpkgExe integrate install
      } finally {
        $WarningPreference = $oldWarning
      }
    } else {
      Write-Warning "vcpkg.exe is present but not a valid PE file; skipping integrate"
    }
  } catch {
    Write-Warning "vcpkg integrate encountered an error: $_"
  }
} else {
  Write-Warning "vcpkg.exe not found; skipping integrate"
}

Write-Host "vcpkg is ready at $vcpkgDir"
Pop-Location
Write-Host "install.ps1 finished"
