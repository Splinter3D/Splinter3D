# Runs the end-to-end tests (tests/end_to_end) against the splinter3D.exe inside a
# release .zip rather than the one in build/, to catch packaging mistakes (missing DLLs,
# assets, wrong layout). Usage: e2e-package.ps1 -Package <package.zip>

param(
  [Parameter(Mandatory = $true)][string]$Package
)

$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$workDir = Join-Path ([System.IO.Path]::GetTempPath()) ("splinter3d-e2e-" + [guid]::NewGuid())

try {
  Expand-Archive -Path $Package -DestinationPath $workDir
  $binary = Get-ChildItem -Path $workDir -Recurse -Filter 'splinter3D.exe' | Select-Object -First 1
  if (-not $binary) {
    Write-Error "No splinter3D.exe found in $Package"
    exit 1
  }
  Write-Host "[e2e-package] testing $($binary.FullName)"

  $env:SPLINTER3D_BIN = $binary.FullName
  Push-Location $projectRoot
  try {
    python -m unittest discover -s tests/end_to_end -v
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  }
  finally {
    Pop-Location
  }
}
finally {
  Remove-Item -Path $workDir -Recurse -Force -ErrorAction SilentlyContinue
}
