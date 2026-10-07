@echo off
rem LookAway Windows Build & Packaging Launcher
rem Automatically bypasses PowerShell ExecutionPolicy restrictions for this process.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_exe.ps1" %*
