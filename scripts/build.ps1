#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not (Get-Command python -ErrorAction SilentlyContinue) -and
    -not (Get-Command python3 -ErrorAction SilentlyContinue)) {
    Write-Host "Python 3 is not installed. Please install Python 3 to continue."
    exit 1
}

if (-not (Get-Command poetry -ErrorAction SilentlyContinue)) {
    Write-Host "Poetry is not installed. Please install Poetry to continue."
    exit 1
}

$Python = if (Get-Command python3 -ErrorAction SilentlyContinue) {
    "python3"
} else {
    "python"
}

poetry run -P "$ScriptDir/Build" -C "$ScriptDir/.." $Python scripts/Build/build.py @args
