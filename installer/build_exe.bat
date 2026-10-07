@echo off
rem LookAway Windows Build & Packaging Launcher
rem Automatically maps UNC network paths (e.g. VMware Shared Folders) to a drive letter
pushd "%~dp0.."
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_exe.ps1" %*
set EXIT_CODE=%ERRORLEVEL%
popd
exit /b %EXIT_CODE%
