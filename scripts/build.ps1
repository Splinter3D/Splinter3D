#!/usr/bin/env pwsh

param(
    [Alias('h')][switch] $Help,
    [Alias('d')][switch] $Debug,
    [Alias('c')][switch] $Clean,
    [Alias('f')][switch] $Fclean,
    [Alias('t')][switch] $Tests,
    [Alias('r')][switch] $Re,
    [switch]$AutoVcpkg,
    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$EMOJI_OK = '✅'
$EMOJI_ERR = '❌'
$EMOJI_INFO = '🚧'
$EMOJI_QUESTION = '❓'

function Fail([string]$msg, [string]$hint = "", [bool]$shouldExit = $True) {
    Write-Host "[$EMOJI_ERR] ERROR:`t$msg" -ForegroundColor Red
    if ($hint) { Write-Host "        $hint" -ForegroundColor DarkRed }
    if ($shouldExit) { exit 84 }
}

function Ok([string]$msg) {
    Write-Host "[$EMOJI_OK] SUCCESS:`t$msg" -ForegroundColor Green
}

function Info([string]$msg) {
    Write-Host "[$EMOJI_INFO] RUNNING:`t$msg" -ForegroundColor Yellow
}

function Prompt-Interactive([string]$message) {
    try {
        return (Read-Host -Prompt ("[$EMOJI_QUESTION] PROMPT:`t{0}" -f $message))
    } catch {
        return ""
    }
}

function Assert-Tool([string]$tool) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        $hint = "please install '$tool'"
        if ($tool -eq 'ninja') {
            $hint = "ninja not found. On Windows you can: choco install ninja -y ; or winget install --id=GnuWin32.Ninja -e"
        }
        Fail "command '$tool' not found" $hint
    }

}

function Get-CPUCount {
    if ($IsWindows) {
        return (Get-WmiObject -Class Win32_Processor).NumberOfLogicalProcessors
    }
    if (Test-Path /proc/cpuinfo) {
        return (Get-Content /proc/cpuinfo | Select-String "^processor" | Measure-Object).Count
    }
    return 4
}

function Ensure-Tools {
    $missing = @()
    foreach ($t in @('git', 'cmake')) {
        if (-not (Get-Command $t -ErrorAction SilentlyContinue)) {
            $missing += $t
        }
    }
    $desiredGenerator = ''
    if ($env:CMAKE_GENERATOR) { $desiredGenerator = $env:CMAKE_GENERATOR }
    elseif ($env:BUILD_SYSTEM) { $desiredGenerator = $env:BUILD_SYSTEM }
    else {
        if (Get-Command ninja -ErrorAction SilentlyContinue) { $desiredGenerator = 'Ninja' } else { $desiredGenerator = 'Unix Makefiles' }
    }
    $requiredBuildTool = ''
    if ($desiredGenerator -like '*Makefiles*') { $requiredBuildTool = 'make' }
    elseif ($desiredGenerator -eq 'Ninja') { $requiredBuildTool = 'ninja' }
    else {
        if (Get-Command ninja -ErrorAction SilentlyContinue) { $requiredBuildTool = 'ninja' }
        elseif (Get-Command make -ErrorAction SilentlyContinue) { $requiredBuildTool = 'make' }
        else { $requiredBuildTool = 'none' }
    }
    if ($requiredBuildTool -eq 'none') { $missing += 'ninja or make' }
    else {
        if (-not (Get-Command $requiredBuildTool -ErrorAction SilentlyContinue)) { $missing += $requiredBuildTool }
    }
    if ($missing.Count -gt 0) {
        Fail "Missing required tools: $($missing -join ', ')" "" $False
        exit 1
    }

    Ok "required tools found (git, cmake, $requiredBuildTool)"
}

function Clone-Vcpkg {
    param(
        [string]$Destination
    )
    if (-not $Destination) { $Destination = "$HOME\vcpkg" }
    if ($Destination.StartsWith('~')) { $Destination = $Destination -replace '^~',$HOME }
    if (-not $Destination.EndsWith('vcpkg')) { $Destination = Join-Path $Destination 'vcpkg' }
    $parent = Split-Path $Destination -Parent
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
    git clone https://github.com/microsoft/vcpkg.git $Destination
    return $Destination
}

function Bootstrap-Vcpkg {
    param(
        [string]$Dir
    )
    if (-not (Test-Path $Dir)) {
        Fail "vcpkg path not found: $Dir" "" $False
        return $false
    }
    Push-Location $Dir
    try {
        if ($IsWindows) {
            .\bootstrap-vcpkg.bat
        } else {
            ./bootstrap-vcpkg.sh
        }
    } finally {
        Pop-Location
    }
    return $true
}

function Get-VcpkgToolchainArgs {
    $root = $env:VCPKG_ROOT
    if (-not $root) { $root = "$HOME\vcpkg" }
    if ($root.StartsWith('~')) { $root = $root -replace '^~',$HOME }
    if (-not (Test-Path "$root/scripts/buildsystems/vcpkg.cmake")) {
        if ($AutoVcpkg) {
            Info "Auto-installing vcpkg into $root"
            $cloned = Clone-Vcpkg -Destination $root
            Bootstrap-Vcpkg -Dir $cloned | Out-Null
        } else {
            $ans = Prompt-Interactive "vcpkg not detected, would you like to install it? (y/n)"
            if ($ans -match '^[Yy]') {
                Info "Installing vcpkg into $root"
                $cloned = Clone-Vcpkg -Destination $root
                Bootstrap-Vcpkg -Dir $cloned | Out-Null
            } else {
                Info "vcpkg toolchain not found at $root; continuing without vcpkg"
                return @()
            }
        }
    }
    if (-not $env:VCPKG_TARGET_TRIPLET) {
        $arch = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()
        if ($IsMacOS) {
            if ($arch -eq 'Arm64') { $env:VCPKG_TARGET_TRIPLET = 'arm64-osx' } else { $env:VCPKG_TARGET_TRIPLET = 'x64-osx' }
        } elseif ($IsLinux) {
            if ($arch -eq 'Arm64') { $env:VCPKG_TARGET_TRIPLET = 'arm64-linux' } else { $env:VCPKG_TARGET_TRIPLET = 'x64-linux' }
        } else {
            if ($arch -eq 'Arm64') { $env:VCPKG_TARGET_TRIPLET = 'arm64-windows' } else { $env:VCPKG_TARGET_TRIPLET = 'x64-windows' }
        }
        Info "Auto-selected VCPKG_TARGET_TRIPLET=$($env:VCPKG_TARGET_TRIPLET)"
    }
    return @("-DCMAKE_TOOLCHAIN_FILE=$root/scripts/buildsystems/vcpkg.cmake", "-DVCPKG_TARGET_TRIPLET=$($env:VCPKG_TARGET_TRIPLET)")
}

function Configure-And-Build {
    param(
        [string]$BuildType = 'Release',
        [string[]]$ExtraArgs
    )
    Ensure-Tools
    git submodule update --init --recursive
    $vcpkgArgs = Get-VcpkgToolchainArgs
    if ($env:CMAKE_GENERATOR) { $generator = $env:CMAKE_GENERATOR }
    elseif ($env:BUILD_SYSTEM) { $generator = $env:BUILD_SYSTEM }
    else { if (Get-Command ninja -ErrorAction SilentlyContinue) { $generator = 'Ninja' } else { $generator = 'Unix Makefiles' } }
    $cmakeArgs = @('-G', $generator, "-DCMAKE_BUILD_TYPE=$BuildType") + $vcpkgArgs + $ExtraArgs
    New-Item -ItemType Directory -Force -Path build | Out-Null
    Push-Location build
    if ($DryRun) { Info "DRY RUN: cmake .. $($cmakeArgs -join ' ')"; Pop-Location; return }
    cmake .. $cmakeArgs
    if ($DryRun) {
        if ($generator -like '*Makefiles*') { Info "DRY RUN: make -j $(Get-CPUCount) splinter"; Pop-Location; return }
        else { Info "DRY RUN: ninja -j $(Get-CPUCount) splinter"; Pop-Location; return }
    }
    if ($generator -like '*Makefiles*') {
        make -j (Get-CPUCount) splinter
    } else {
        ninja -j (Get-CPUCount) splinter
    }
    $rc = $LASTEXITCODE
    Pop-Location
    exit $rc
}

function Invoke-BuildRelease {
    Configure-And-Build -BuildType 'Release'
}

function Invoke-BuildDebug {
    Configure-And-Build -BuildType 'Debug' -ExtraArgs @('-DENABLE_DEBUG=ON')
}

function Invoke-RunTests {
    Ensure-Tools
    git submodule update --init --recursive
    $vcpkgArgs = Get-VcpkgToolchainArgs
    New-Item -ItemType Directory -Force -Path build | Out-Null
    Push-Location build
    if ($env:CMAKE_GENERATOR) { $generator = $env:CMAKE_GENERATOR }
    elseif ($env:BUILD_SYSTEM) { $generator = $env:BUILD_SYSTEM }
    else { if (Get-Command ninja -ErrorAction SilentlyContinue) { $generator = 'Ninja' } else { $generator = 'Unix Makefiles' } }
    if ($DryRun) { Info "DRY RUN: cmake .. -G $generator -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON $($vcpkgArgs -join ' ')"; Pop-Location; return }
    cmake .. -G $generator -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON $vcpkgArgs
    if ($generator -like '*Makefiles*') { make -j (Get-CPUCount) splinter_unit_tests } else { ninja -j (Get-CPUCount) splinter_unit_tests }
    Pop-Location
    if ($DryRun) { Info 'Dry run; would run unit tests'; return }
    & ./splinter_unit_tests
    Ok "unit tests executed"

    $coverageOut = "code_coverage.txt"
    try {
        if (Get-Command gcovr -ErrorAction SilentlyContinue) {
            Info "generating coverage (gcovr)"
            gcovr -r . --exclude tests/ | Out-File -Encoding utf8 $coverageOut
        } else {
            "Coverage not available on this runner." | Out-File -Encoding utf8 $coverageOut
        }
    } catch {
        "Coverage step failed or unavailable." | Out-File -Encoding utf8 $coverageOut
    }
    Get-Content $coverageOut | Write-Host
}

function Invoke-Clean() {
    $scriptRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Path $MyInvocation.MyCommand.Path -Parent }
    $buildDir = Join-Path $scriptRoot 'build'
    if (Test-Path $buildDir) {
        Info "Removing build directory: ${buildDir}"
        try {
            Remove-Item -LiteralPath $buildDir -Recurse -Force -ErrorAction Stop
            Info "Removed: ${buildDir}"
        } catch {
            Fail "Failed to remove ${buildDir}: $($_.Exception.Message)" "" $False
        }
    } else {
        Info "No build directory at: ${buildDir}"
    }
}

function Invoke-FClean() {
    Invoke-Clean
    $paths = @(
        "*.so","*.dylib", "*.dll","*.lib","*.a","*.exp",
        "splinter_unit_tests","splinter_unit_tests.exe","splinter",
        "splinter.exe","unit_tests","plugins","code_coverage.txt",
        "unit_tests-*.profraw","unit_tests.profdata","vgcore*","cmake-build-debug"
    )
    foreach ($p in $paths) { Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $p }
    if (Test-Path libs) { Get-ChildItem libs | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue }
}

if ($Help) {
@"
USAGE:
  .\build.ps1       builds splinter project (Ninja)

ARGUMENTS:
  .\build.ps1 [-h]        displays this message
  .\build.ps1 [-d]        debug flags compilation
  .\build.ps1 [-c]        clean the project
  .\build.ps1 [-f]        fclean the project
  .\build.ps1 [-t]        run unit tests
  .\build.ps1 [-r]        fclean then rebuild (release)
"@
    exit 0
}

if     ($Clean)  { Invoke-Clean;  exit 0 }
elseif ($Fclean) { Invoke-FClean; exit 0 }
elseif ($Debug)    { Invoke-FClean; Invoke-BuildDebug; exit 0 }
elseif ($Tests)    { Invoke-RunTests;   exit 0 }
elseif ($Re)       { Invoke-FClean; Invoke-BuildRelease; exit 0 }
else               { Invoke-BuildRelease }
