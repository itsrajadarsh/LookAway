# Building LookAway Windows Installer (EXE)

Step-by-step guide to compiling **LookAway** in Release mode, generating the setup installer wizard (`LookAway-Setup-v2.0.0.exe`), and publishing GitHub releases.

---

## 📋 Table of Contents

- [⚙️ Environment Setup](#️-environment-setup)
- [🔨 Step 1: Build the Release Executable](#-step-1-build-the-release-executable)
- [📦 Step 2: Deploy Qt Runtime Dependencies](#-step-2-deploy-qt-runtime-dependencies)
- [💿 Step 3: Create the Windows Installer](#-step-3-create-the-windows-installer)
- [🚀 Publishing a GitHub Release](#-publishing-a-github-release)
- [⚡ Quick All-In-One Script](#-quick-all-in-one-script)

---

## ⚙️ Environment Setup

Before running commands in PowerShell or Terminal, update your `PATH` variable so CMake, GCC/MinGW, Qt, and Inno Setup are recognized:

```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\CMake_64\bin;C:\Qt\6.8.3\mingw_64\bin;C:\Users\Adarsh\AppData\Local\Programs\Inno Setup 6;" + $env:PATH
```

> [!NOTE]
> Ensure paths match your local installation directories for Qt 6.8.3 and Inno Setup 6.

---

## 🔨 Step 1: Build the Release Executable

Compile an optimized **Release** build with debug symbols stripped and compiler optimizations enabled:

```powershell
# 1. Create and configure build directory for Release
cmake -B build-windows/Release -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.8.2/mingw_64"

# 2. Compile the executable
cmake --build build-windows/Release --config Release
```

**Artifact Output:** `build-windows/Release/LookAway.exe`

---

## 📦 Step 2: Deploy Qt Runtime Dependencies

Qt applications require runtime `.dll` files and plugins (e.g., `Qt6Core`, `Qt6Widgets`, `Multimedia` plugins) to run outside of Qt Creator.

Run `windeployqt` to copy all required DLLs into the `build-windows/Release/` directory automatically:

```powershell
windeployqt build-windows/Release/LookAway.exe
```

> [!TIP]
> After running `windeployqt`, `build-windows/Release/LookAway.exe` becomes a standalone executable and can be tested directly without needing Qt installed.

---

## 💿 Step 3: Create the Windows Installer

Bundle the compiled binary and dependent assets into a single setup wizard (`LookAway-Setup-v2.0.0.exe`) using **Inno Setup** (`ISCC.exe`).

Run the compiler against the `.iss` script:

```powershell
ISCC.exe installer/build_exe.iss
```

**Installer Output:** `installer_output/windows/LookAway-Setup-v2.0.0.exe`

---

## 🚀 Publishing a GitHub Release

When ready to publish a release for users on GitHub:

### 1. Tag and Push to Git
```bash
git add .
git commit -m "Release v2.0.0"
git tag -a v2.0.0 -m "LookAway v2.0.0 Release"
git push origin main --tags
```

### 2. Upload to GitHub
1. Navigate to your repository on GitHub.
2. Click **Releases** → **Draft a new release**.
3. Select tag `v2.0.0`.
4. Set Title to `LookAway v2.0.0`.
5. Drag and drop **`installer_output/windows/LookAway-Setup-v2.0.0.exe`** under *Attach binaries by dropping them here*.
6. Click **Publish release**.

---

## ⚡ Quick All-In-One Script

Save the following code as `build_release.ps1` in your repository root to automate the build and installer generation process:

```powershell
# Set environment paths
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\CMake_64\bin;C:\Qt\6.8.3\mingw_64\bin;C:\Users\Adarsh\AppData\Local\Programs\Inno Setup 6;" + $env:PATH

# Stop running instances
Stop-Process -Name "LookAway" -Force -ErrorAction SilentlyContinue

# Configure & Build
Write-Host "Building Release..." -ForegroundColor Cyan
cmake -B build-windows/Release -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.8.2/mingw_64"
cmake --build build-windows/Release --config Release

# Deploy DLLs
Write-Host "Deploying Qt DLLs..." -ForegroundColor Cyan
windeployqt build-windows/Release/LookAway.exe

# Generate Installer
Write-Host "Compiling Installer..." -ForegroundColor Cyan
ISCC.exe installer/build_exe.iss

Write-Host "Done! Installer created at installer_output/windows/LookAway-Setup-v2.0.0.exe" -ForegroundColor Green
```
