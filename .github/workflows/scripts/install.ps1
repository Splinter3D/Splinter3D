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
  Write-Host "vcpkg.exe not found; running bootstrap"
#   try {
#     curl.exe -L -o $vcpkgExe "https://github.com/microsoft/vcpkg/releases/latest/download/vcpkg.exe"
#   } catch {
#     Write-Warning "curl download failed: $_"
#   }

#   # Validate downloaded file looks like a Windows PE (MZ header)
#   $validExe = $false
#   if (Test-Path $vcpkgExe) {
#     try {
#       $hdr = Get-Content -Path $vcpkgExe -Encoding Byte -TotalCount 2 -ErrorAction Stop
#       if ($hdr.Count -ge 2 -and $hdr[0] -eq 0x4d -and $hdr[1] -eq 0x5a) { $validExe = $true }
#     } catch { }
#   }

#   if (-not $validExe) {
#     Write-Warning "Downloaded vcpkg.exe is not a valid PE file; attempting Releases API download"
#     try {
#       $api = 'https://api.github.com/repos/microsoft/vcpkg/releases/latest'
#       $rel = Invoke-RestMethod -Uri $api -UseBasicParsing
#       $asset = $rel.assets | Where-Object { $_.name -eq 'vcpkg.exe' } | Select-Object -First 1
#       if ($null -ne $asset) {
#         $url = $asset.browser_download_url
#         curl.exe -L -o $vcpkgExe $url
#         try {
#           $hdr2 = Get-Content -Path $vcpkgExe -Encoding Byte -TotalCount 2 -ErrorAction Stop
#           if ($hdr2.Count -ge 2 -and $hdr2[0] -eq 0x4d -and $hdr2[1] -eq 0x5a) { $validExe = $true }
#         } catch { }
#       } else {
#         Write-Warning "No vcpkg.exe asset found in latest release"
#       }
#     } catch {
#       Write-Warning "Releases API download attempt failed: $_"
#     }
#   }
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
    # Double-check header before running
    $hdr = Get-Content -Path $vcpkgExe -Encoding Byte -TotalCount 2 -ErrorAction Stop
    if ($hdr.Count -ge 2 -and $hdr[0] -eq 0x4d -and $hdr[1] -eq 0x5a) {
      & $vcpkgExe integrate install
    } else {
      Write-Warning "vcpkg.exe is present but not a valid PE file; skipping integrate"
    }
  } catch {
    Write-Warning "vcpkg integrate failed: $_"
  }
} else {
  Write-Warning "vcpkg.exe not found; skipping integrate"
}

Write-Host "vcpkg is ready at $vcpkgDir"
Pop-Location
Write-Host "install.ps1 finished"
