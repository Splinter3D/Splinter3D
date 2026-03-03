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
  [switch]$UseCPack,
  [switch]$DryRun
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

# Ensure vcpkg exists and is bootstrapped (clone full repo and bootstrap like Windows-Install)
if (-not $DryRun) {
  if (-not (Test-Path $VcpkgDir)) {
    $vcpkgParent = Split-Path $VcpkgDir -Parent
    if (-not (Test-Path $vcpkgParent)) { New-Item -ItemType Directory -Path $vcpkgParent | Out-Null }
    Set-Location $vcpkgParent
    $cloneUrl = 'https://github.com/microsoft/vcpkg.git'
    $maxAttempts = 3
    $attempt = 0
    $cloned = $false
    while (-not $cloned -and $attempt -lt $maxAttempts) {
      $attempt++
      Write-Host "Cloning vcpkg into $VcpkgDir... (attempt $attempt/$maxAttempts)"
      git clone $cloneUrl $VcpkgDir
      if ($LASTEXITCODE -eq 0 -and (Test-Path $VcpkgDir)) { $cloned = $true; break }
      Write-Warning "git clone exited with code $LASTEXITCODE."
      if ($attempt -lt $maxAttempts) {
        $sleep = [math]::Pow(2, $attempt)
        Write-Host "Retrying in ${sleep}s..."
        Start-Sleep -Seconds $sleep
      }
    }
    if (-not $cloned) { Fail "Failed to clone vcpkg into $VcpkgDir after $maxAttempts attempts" }
  }

  Push-Location $VcpkgDir
  Write-Host "Bootstrapping vcpkg..."
  & .\bootstrap-vcpkg.bat
  if ($LASTEXITCODE -ne 0) { Fail "vcpkg bootstrap failed (exit code $LASTEXITCODE)" }

  & .\vcpkg integrate install
  if ($LASTEXITCODE -ne 0) { Write-Warning "vcpkg integrate install failed (exit code $LASTEXITCODE)" }

  # Diagnostic: list vcpkg scripts folder so CI logs show its contents
  $scriptsDir = Join-Path $VcpkgDir 'scripts\buildsystems'
  Write-Host "vcpkg dir: $VcpkgDir"
  if (Test-Path $scriptsDir) {
      Write-Host "Listing $scriptsDir contents:"
      Get-ChildItem -Path $scriptsDir | ForEach-Object { Write-Host " - $($_.Name)" }
  } else {
      Write-Warning "$scriptsDir does not exist after bootstrap"
  }
  Pop-Location
} else {
  Write-Host "DryRun: skipping vcpkg clone/bootstrap and integrate"
}

if (-not $DryRun) {
  Write-Host "Installing vcpkg dependencies (classic mode per vcpkg.json)..."
  $vcpkgExe = (Join-Path $VcpkgDir 'vcpkg.exe')
  if (Test-Path (Join-Path $RepoRoot 'vcpkg.json')) {
    try {
      Write-Host "Using classic vcpkg install for dependencies listed in vcpkg.json"
      $json = Get-Content (Join-Path $RepoRoot 'vcpkg.json') -Raw | ConvertFrom-Json
      if ($json.dependencies) {
        foreach ($dep in $json.dependencies) {
          Write-Host "Installing $dep via vcpkg classic mode"
          & $vcpkgExe install $dep --triplet $Triplet
          if ($LASTEXITCODE -ne 0) { Write-Warning "vcpkg install $dep failed with exit code $LASTEXITCODE; continuing" }
        }
      } else {
        Write-Host "No dependencies array found in vcpkg.json"
      }
    } catch {
      Write-Warning "vcpkg classic install failed: $_"
    }
  } else {
    Write-Host "No vcpkg.json found; skipping dependency installs."
  }
  Pop-Location
} else {
  Write-Host "DryRun: skipping vcpkg dependency installs"
  Pop-Location
}

# Prepare build directory
$buildDir = Join-Path $RepoRoot "build"
if (Test-Path $buildDir) { Remove-Item -Recurse -Force $buildDir }
New-Item -ItemType Directory -Path $buildDir | Out-Null

$toolchain = Join-Path $VcpkgDir "scripts\buildsystems\vcpkg.cmake"
if (-not (Test-Path $toolchain)) {
  if (-not $DryRun) {
    Fail "vcpkg toolchain not found at $toolchain"
  } else {
    Write-Warning "vcpkg toolchain not found at $toolchain (DryRun mode: continuing to write init file for diagnostics)"
  }
}

Write-Host "Configuring CMake with generator fallback..."
$cmakeExe = "cmake"
$generators = @(
  @{ Name = 'Visual Studio 17 2022'; Args = @('-G', 'Visual Studio 17 2022', '-A', 'x64') },
  @{ Name = 'Visual Studio 16 2019'; Args = @('-G', 'Visual Studio 16 2019', '-A', 'x64') },
  @{ Name = 'Ninja'; Args = @('-G', 'Ninja') }
)

$selectedGenerator = $null
# verify toolchain exists
if (-not (Test-Path $toolchain)) {
  if (-not $DryRun) {
    Write-Warning "vcpkg toolchain file not found at $toolchain"
    Write-Warning "If you expect a full vcpkg repo here, ensure you cloned the repository (not only downloaded vcpkg.exe)."
    Fail "vcpkg toolchain file missing"
  } else {
    Write-Warning "vcpkg toolchain file not found at $toolchain (DryRun: continuing)"
  }
} else {
  Write-Host "Toolchain file present: $toolchain"
  Write-Host "First lines of toolchain file (for diagnostics):"
  Get-Content -Path $toolchain -TotalCount 8 | ForEach-Object { Write-Host "  $_" }
}

foreach ($gen in $generators) {
  $sanitized = ($gen.Name -replace '[^A-Za-z0-9_-]', '_')
  $candidateBuild = Join-Path $RepoRoot ("build_$sanitized")
  if (Test-Path $candidateBuild) { Remove-Item -Recurse -Force $candidateBuild -ErrorAction SilentlyContinue }
  New-Item -ItemType Directory -Path $candidateBuild | Out-Null
  # Create an initial cache file to avoid passing complex -D arguments which can be mis-parsed
  $initFile = Join-Path $candidateBuild 'vcpkg_init.cmake'
  # Use forward slashes in paths to avoid CMake escaping issues on Windows (\a etc.)
  $toolchainPosix = $toolchain -replace '\\','/'
  $installPosix = $InstallPrefix -replace '\\','/'
  $initContents = @(
    'set(CMAKE_TOOLCHAIN_FILE "' + $toolchainPosix + '" CACHE STRING "Vcpkg toolchain")',
    'set(CMAKE_INSTALL_PREFIX "' + $installPosix + '" CACHE PATH "Install prefix")',
    'set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type")'
  )
  # Write the initial cache file using WriteAllLines so each array element becomes a separate line
  # with the platform newline (CRLF on Windows). This avoids CMake parse errors when using -C <file>.
  try {
    # Build the init file as a here-string to guarantee real newlines between each set() entry.
    $text = @"
set(CMAKE_TOOLCHAIN_FILE "$toolchainPosix" CACHE STRING "Vcpkg toolchain")
set(CMAKE_INSTALL_PREFIX "$installPosix" CACHE PATH "Install prefix")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type")
"@
    $bytes = [System.Text.Encoding]::ASCII.GetBytes($text)
    [System.IO.File]::WriteAllBytes($initFile, $bytes)
  } catch {
    Write-Warning "Failed to write init file via here-string WriteAllBytes: $_; falling back to Out-File"
    $initContents | Out-File -FilePath $initFile -Encoding ASCII -Force
  }

  # Emit the first lines of the generated init file for diagnostics
  Write-Host "Generated init file: $initFile (first 8 lines)"
  Get-Content -Path $initFile -TotalCount 8 | ForEach-Object { Write-Host "  $_" }

  # Dump raw bytes (hex) of the init file to help diagnose line-ending / encoding issues that
  # cause CMake to report "Expected a newline, got identifier 'set'". Show first 128 bytes.
  try {
    $raw = [System.IO.File]::ReadAllBytes($initFile)
    $len = $raw.Length
    Write-Host "Init file byte-length: $len"
    if ($DryRun) {
      # In DryRun print the full hex to make diagnostics definitive
      if ($len -gt 0) {
        $hex = ($raw | ForEach-Object { $_.ToString('x2') }) -join ' '
        Write-Host "Init file full bytes (hex):"
        Write-Host "  $hex"
      } else {
        Write-Host "Init file is empty"
      }
    } else {
      $previewCount = [math]::Min(128, $len)
      if ($previewCount -gt 0) {
        $hex = ($raw[0..($previewCount - 1)] | ForEach-Object { $_.ToString('x2') }) -join ' '
        Write-Host "Init file first $previewCount bytes (hex):"
        Write-Host "  $hex"
      } else {
        Write-Host "Init file is empty"
      }
    }
  } catch {
    Write-Warning "Failed to read init file bytes for diagnostic: $_"
  }

  if ($DryRun) {
    Write-Host "DryRun: wrote init file and dumped bytes in $candidateBuild; skipping CMake configure/build"
    exit 0
  }

  $cmakeArgs = @('-S', $RepoRoot, '-B', $candidateBuild) + $gen.Args + @('-C', $initFile)
  Write-Host "Trying generator: $($gen.Name)"
  # Build a single safely-quoted argument string to avoid Start-Process splitting generator name
  $argString = ($cmakeArgs | ForEach-Object {
    if ($_ -match '\s') { '"' + $_ + '"' } else { $_ }
  }) -join ' '
  Write-Host "$cmakeExe $argString"
  # Use Start-Process with a single argument string and capture output
  $proc = Start-Process -FilePath $cmakeExe -ArgumentList $argString -NoNewWindow -PassThru -Wait -RedirectStandardOutput "$candidateBuild\cmake_configure.stdout.txt" -RedirectStandardError "$candidateBuild\cmake_configure.stderr.txt"
  $exit = $proc.ExitCode
  Get-Content "$candidateBuild\cmake_configure.stdout.txt" -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "cmake> $_" }
  Get-Content "$candidateBuild\cmake_configure.stderr.txt" -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "cmake-err> $_" }
  if ($exit -eq 0) {
    $selectedGenerator = $gen.Name
    $buildDir = $candidateBuild
    break
  } else {
    Write-Warning "Generator $($gen.Name) failed with exit code $exit"
  }
}

if (-not $selectedGenerator) { Fail "CMake configuration failed for all candidate generators" }

Write-Host "Selected generator: $selectedGenerator (build dir: $buildDir)"

Write-Host "Building and installing..."
  if ($selectedGenerator -eq 'Ninja') {
    & $cmakeExe --build $buildDir --config Release --parallel 4
    if ($LASTEXITCODE -ne 0) { Fail "Build/install failed (Ninja)" }
  } else {
    & $cmakeExe --build $buildDir --config Release --target INSTALL -- /m:4
    if ($LASTEXITCODE -ne 0) { Fail "Build/install failed (MSBuild)" }
  }

# Ensure staging directory exists
if (-not (Test-Path $InstallPrefix)) { New-Item -ItemType Directory -Path $InstallPrefix -Force | Out-Null }

# Copy locale files into staging
if (Test-Path (Join-Path $RepoRoot 'locale')) {
    Write-Host "Copying locale/ into staging"
    $destLocale = Join-Path $InstallPrefix 'locale'
    Remove-Item -Recurse -Force $destLocale -ErrorAction SilentlyContinue
    Copy-Item -Path (Join-Path $RepoRoot 'locale') -Destination $destLocale -Recurse -Force
}

# Copy assets (icons, etc.) into staging
if (Test-Path (Join-Path $RepoRoot 'assets')) {
    Write-Host "Copying assets/ into staging"
    $destAssets = Join-Path $InstallPrefix 'assets'
    Remove-Item -Recurse -Force $destAssets -ErrorAction SilentlyContinue
    Copy-Item -Path (Join-Path $RepoRoot 'assets') -Destination $destAssets -Recurse -Force
}

# Attempt to locate the built executable and copy into staging/bin
$exeName = 'splinter3D.exe'
$found = Get-ChildItem -Path $buildDir -Recurse -Filter $exeName -ErrorAction SilentlyContinue | Select-Object -First 1
if ($found) {
    $binDir = Join-Path $InstallPrefix 'bin'
    if (-not (Test-Path $binDir)) { New-Item -ItemType Directory -Path $binDir | Out-Null }
    Copy-Item -Path $found.FullName -Destination (Join-Path $binDir $exeName) -Force
    Write-Host "Copied executable to staging/bin"
} else {
    Write-Warning "Executable $exeName not found in build tree; ensure CMake install rules place binaries into staging or adjust script to copy them explicitly."
}

if ($UseCPack) {
    Write-Host "Running CPack (requires NSIS or other packager)"
    Push-Location $buildDir
    try {
      & cpack -G NSIS
      if ($LASTEXITCODE -ne 0) { Write-Warning "CPack/NSIS failed (exit code $LASTEXITCODE)" }
    } catch {
      Write-Warning "CPack/NSIS failed or NSIS not installed: $_"
    }
    Pop-Location
}

# Create ZIP archive of staging tree
$zipPath = Join-Path $OutDir "release-package.zip"
if (Test-Path $zipPath) { Remove-Item $zipPath }
Write-Host "Creating ZIP: $zipPath"
try {
  Compress-Archive -Path (Join-Path $InstallPrefix '*') -DestinationPath $zipPath -Force
} catch {
  Fail "Failed to create ZIP: $_"
}

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
