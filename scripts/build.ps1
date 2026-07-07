#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not (Get-Command python -ErrorAction SilentlyContinue) -and
    -not (Get-Command python3 -ErrorAction SilentlyContinue)) {
    Write-Host "Python 3 is not installed. Please install Python 3 to continue."
    exit 1
}

$Python = if (Get-Command python -ErrorAction SilentlyContinue) {
    "python"
} else {
    "python3"
}

cd $ScriptDir\..

& $Python .\scripts\Build\build.py @args
