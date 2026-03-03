param(
    [string]$ZipUrl = "",
    [string]$LocalZip = "",
    [string]$OutDir = "C:\Program Files\splinter3D",
    [string]$ExpectedSha256 = "",
    [string]$FreeCADInstaller = ""
)

function Run-As-Administrator {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    # In CI environments we must not attempt elevation
    if ($env:GITHUB_ACTIONS -eq 'true' -or $env:CI -eq 'true') {
        Write-Host "CI runner detected - skipping elevation"
        return
    }

    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
        Write-Host "Restarting script as administrator..."
        Start-Process powershell.exe -Verb RunAs -ArgumentList @(
            "-NoProfile",
            "-ExecutionPolicy", "Bypass",
            "-File", $PSCommandPath,
            "-ZipUrl", $ZipUrl,
            "-LocalZip", $LocalZip,
            "-OutDir", $OutDir,
            "-ExpectedSha256", $ExpectedSha256,
            "-FreeCADInstaller", $FreeCADInstaller
        )
        exit
    }
}

function Download-Zip($url, $outPath) {
    Write-Host "Downloading $url -> $outPath"
    try {
        Invoke-WebRequest -Uri $url -OutFile $outPath -UseBasicParsing -ErrorAction Stop
        return $true
    } catch {
        Write-Error "Failed to download $url: $_"
        return $false
    }
}

function Compute-SHA256($path) {
    if (-not (Test-Path $path)) { return "" }
    $h = Get-FileHash -Path $path -Algorithm SHA256
    return $h.Hash.ToLowerInvariant()
}

function Install-FreeCAD($InstallerPath) {
    if (-not $InstallerPath) { return }
    Write-Host "Checking FreeCAD installation..."
    $userHome = [Environment]::GetFolderPath("UserProfile")
    $binPath = "bin\FreeCAD.exe"
    $searchDirs = @(
        "$userHome\AppData\Local\Programs\FreeCAD *\$binPath",
        "C:\Program Files\FreeCAD *\$binPath",
        "C:\Program Files (x86)\FreeCAD *\$binPath"
    )
    $found = $false
    foreach ($pattern in $searchDirs) {
        $matches = Get-ChildItem -Path $pattern -File -ErrorAction SilentlyContinue
        if ($matches) { $found = $true; break }
    }
    if (-not $found) {
        if (Test-Path $InstallerPath) {
            Write-Host "Installing FreeCAD from $InstallerPath..."
            Start-Process -FilePath $InstallerPath -ArgumentList "/SILENT" -Wait
        } else {
            Write-Warning "FreeCAD installer not found at $InstallerPath"
        }
    } else { Write-Host "FreeCAD already installed. Skipping." }
}

function Copy-StagingToDestination($staging, $dest) {
    Write-Host "Copying files to $dest"
    if (-not (Test-Path $dest)) { New-Item -ItemType Directory -Force -Path $dest | Out-Null }
    Copy-Item -Path (Join-Path $staging '*') -Destination $dest -Recurse -Force
}

function Create-Shortcut($ExePath, $ShortcutPath) {
    $WScriptShell = New-Object -ComObject WScript.Shell
    $shortcut = $WScriptShell.CreateShortcut($ShortcutPath)
    $shortcut.TargetPath = $ExePath
    $shortcut.WorkingDirectory = Split-Path $ExePath
    $shortcut.Save()
}

Run-As-Administrator

Write-Host "Starting installer script"

$tmp = Join-Path $env:TEMP "splinter3d_install_$(Get-Random)"
New-Item -ItemType Directory -Force -Path $tmp | Out-Null

if ($ZipUrl) {
    $tmpZip = Join-Path $tmp "package.zip"
    if (-not (Download-Zip $ZipUrl $tmpZip)) { exit 1 }
} elseif ($LocalZip) {
    if (-not (Test-Path $LocalZip)) { Write-Error "Local zip not found: $LocalZip"; exit 1 }
    $tmpZip = $LocalZip
} else {
    Write-Error "Either -ZipUrl or -LocalZip must be provided"; exit 1
}

if ($ExpectedSha256) {
    $computed = Compute-SHA256 $tmpZip
    if ($computed -ne $ExpectedSha256.ToLowerInvariant()) {
        Write-Error "SHA256 mismatch: expected $ExpectedSha256, got $computed"; exit 1
    } else { Write-Host "SHA256 verified" }
}

$extract = Join-Path $tmp "extracted"
New-Item -ItemType Directory -Force -Path $extract | Out-Null
Write-Host "Extracting $tmpZip -> $extract"
Expand-Archive -Path $tmpZip -DestinationPath $extract -Force

# If archive contains a single top-level folder, use it
$children = Get-ChildItem -Path $extract | Where-Object { $_.PSIsContainer }
if ($children.Count -eq 1) { $staging = $children[0].FullName } else { $staging = $extract }

Copy-StagingToDestination -staging $staging -dest $OutDir

Write-Host "Creating Desktop shortcut..."
$exe = Join-Path $OutDir "bin\splinter3D.exe"
if (-not (Test-Path $exe)) { $exe = Get-ChildItem -Path $OutDir -Recurse -Filter "*splinter*.exe" | Select-Object -First 1 | ForEach-Object { $_.FullName } }
if (Test-Path $exe) {
    $desktop = Join-Path ([Environment]::GetFolderPath("Desktop")) "splinter3D.lnk"
    Create-Shortcut -ExePath $exe -ShortcutPath $desktop
    Write-Host "Desktop shortcut created: $desktop"
} else { Write-Warning "Executable not found to create shortcut." }

if ($FreeCADInstaller) { Install-FreeCAD -InstallerPath $FreeCADInstaller }

Write-Host "Installation complete. Cleaning temporary files."
Remove-Item -Recurse -Force $tmp

Write-Host "Done."
# Avoid interactive pause in CI
if (-not ($env:GITHUB_ACTIONS -eq 'true' -or $env:CI -eq 'true')) {
    Read-Host -Prompt "Press ENTER to exit"
}
