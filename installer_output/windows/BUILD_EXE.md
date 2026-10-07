# Building LookAway Windows Installer (EXE)

Comprehensive guide to building **LookAway** in Release mode, generating the setup installer wizard (`LookAway-Setup-v2.0.0.exe`), and publishing GitHub releases. This guide covers both **native Windows development** and **cross-building inside a Windows Virtual Machine (VMware / VirtualBox)** with shared folders.

---

## 📋 Table of Contents

- [Prerequisites & Required Components](#-prerequisites--required-components)
- [⚡ 1-Step Automated Build (Recommended)](#-1-step-automated-build-recommended)
- [🖥️ Building via Windows VM & Shared Folders](#️-building-via-windows-vm--shared-folders)
- [🔨 Manual Step-by-Step Pipeline](#-manual-step-by-step-pipeline)
- [🚀 Publishing a GitHub Release](#-publishing-a-github-release)
- [💡 Troubleshooting & Tips](#-troubleshooting--tips)

---

## 📋 Prerequisites & Required Components

Install the required development toolchain using the **Qt Online Installer** (Custom Installation):

### 1. Under `Qt 6.8.3` (or your target Qt 6.x):
* ☑️ **MinGW 13.1.0 64-bit** *(Core Qt runtime, base libraries & Widgets)*
* ☑️ **Qt Multimedia** *(under **Additional Libraries** — mandatory for `Qt6::Multimedia` / `QSoundEffect` audio chime playback)*

### 2. Under `Developer and Designer Tools` (Build Tools):
* ☑️ **MinGW 13.1.0 64-bit** *(GCC C++ compiler toolchain & binutils)*
* ☑️ **CMake 3.30+** *(Build system generator)*
* ☑️ **Ninja 1.12+** *(High-performance parallel build driver)*

### 3. Packaging Software:
* ☑️ **Inno Setup 6.x** *(download and install from [jrsoftware.org](https://jrsoftware.org/isdl.php))*
  *(Default path: `C:\Program Files (x86)\Inno Setup 6` or `C:\Users\<Username>\AppData\Local\Programs\Inno Setup 6`)*

> 🖼️ **Visual Reference:** See [qt_installation_requirements.png](../../screenshots/qt_installation_requirements.png) for the exact Qt installer component checklist.

---

## ⚡ 1-Step Automated Build (Recommended)

From PowerShell or Command Prompt at the repository root, run the automated batch launcher:

```cmd
.\installer\build_exe.bat
```

> [!TIP]
> **Why `build_exe.bat`?**
> Windows restricts running unsigned PowerShell `.ps1` scripts by default (`Restricted` / `RemoteSigned`). Running `build_exe.bat` automatically launches the packaging script with `ExecutionPolicy Bypass`, auto-discovers all build tools, and compiles without needing manual permission changes.

### Automated Options:
```cmd
# Standard full build and packaging:
.\installer\build_exe.bat

# Fast re-packaging without recompiling (uses existing binary):
.\installer\build_exe.bat -NoBuild

# Clean re-compile from scratch:
.\installer\build_exe.bat -Clean

# Custom build output directory:
.\installer\build_exe.bat -BuildDir "C:\LookAwayBuild"
```

**Artifact Output:** `installer_output\windows\LookAway-Setup-v2.0.0.exe`

---

## 🖥️ Building via Windows VM & Shared Folders

If you develop on Linux and use a Windows Virtual Machine (VMware / VirtualBox) with Shared Folders to produce the Windows installer:

```
┌─────────────────────────────────────────────────────────────┐
│ Linux Host Machine                                          │
│   └── Repo Root: /home/user/.../Projects/LookAway           │
│         └── installer_output/windows/ (Auto-receives .exe)  │
└──────────────────────────┬──────────────────────────────────┘
                           │ Shared Folder
┌──────────────────────────▼──────────────────────────────────┐
│ Windows Virtual Machine (Guest)                             │
│   ├── Access Repo: \\vmware-host\Shared Folders\...\LookAway│
│   ├── Compile Locally: C:\LookAwayBuildVM (Fast local SSD)  │
│   └── Run Inno Setup: Emits LookAway-Setup-v2.0.0.exe       │
└─────────────────────────────────────────────────────────────┘
```

1. **Open PowerShell in the shared folder** (e.g. `\\vmware-host\Shared Folders\Projects\LookAway`).
2. **Run the batch script:**
   ```cmd
   .\installer\build_exe.bat
   ```
3. **What happens automatically:**
   - Detects the network UNC share path.
   - Sets the compilation target to fast local SSD storage (`C:\LookAwayBuildVM`) to prevent network file-locking and SMB latency.
   - Compiles with Ninja and runs `windeployqt`.
   - Packages with Inno Setup and outputs `LookAway-Setup-v2.0.0.exe` **directly back into your Linux host's `installer_output/windows/` directory**.

---

## 🔨 Manual Step-by-Step Pipeline

If building step-by-step manually without the automated script:

### Step 1: Set Environment PATH
```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\CMake_64\bin;C:\Qt\6.8.3\mingw_64\bin;C:\Program Files (x86)\Inno Setup 6;" + $env:PATH
```

### Step 2: Configure & Compile (Release)
```powershell
# 1. Configure
cmake -S . -B build-windows/Release -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"

# 2. Compile
cmake --build build-windows/Release --config Release
```

### Step 3: Deploy Qt Runtime Dependencies
```powershell
windeployqt build-windows/Release/LookAway.exe --compiler-runtime --no-translations
```

### Step 4: Compile Inno Setup Installer
```powershell
ISCC.exe /DSourceDir="build-windows/Release" installer/build_exe.iss
```

*(If compiling in a VM to `C:\LookAwayBuildVM`, replace `build-windows/Release` with `C:\LookAwayBuildVM`).*

---

## 🚀 Publishing a GitHub Release

When publishing a new release:

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
4. Title: `LookAway v2.0.0`.
5. Drag and drop **`installer_output/windows/LookAway-Setup-v2.0.0.exe`** under *Attach binaries by dropping them here*.
6. Click **Publish release**.

---

## 💡 Troubleshooting & Tips

| Problem | Cause | Solution |
| :--- | :--- | :--- |
| `File ... cannot be loaded because running scripts is disabled` | Windows ExecutionPolicy | Use `.\installer\build_exe.bat` instead of `.ps1`, or run `powershell -ExecutionPolicy Bypass -File .\installer\build_exe.ps1` |
| `CMD does not support UNC paths` | Network share folder | Map a drive letter: `net use Z: "\\vmware-host\Shared Folders\Projects\LookAway"` and `cd Z:\` |
| `ISCC: command not found` | Inno Setup not in PATH | Install Inno Setup 6. `build_exe.bat` auto-discovers both `C:\Program Files (x86)\Inno Setup 6` and `%LOCALAPPDATA%` |
| Audio doesn't play after install | Missing Qt Multimedia | Ensure `Qt Multimedia` was selected in the Qt Online Installer under Additional Libraries |
