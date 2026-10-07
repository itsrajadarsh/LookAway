<#
.SYNOPSIS
    Automated Windows Release build, Qt deployment, and Inno Setup installer script for LookAway.
.DESCRIPTION
    Builds LookAway with CMake & Ninja, deploys Qt 6 runtime dependencies with windeployqt,
    and packages the standalone setup installer wizard into installer_output/windows/.
    Works seamlessly both on native Windows and inside Windows Virtual Machines (VMware/VirtualBox)
    with shared folders.
.PARAMETER BuildDir
    Directory where binaries are compiled. Defaults to "C:\LookAwayBuildVM" if running from a
    network/VMware shared folder, or "<repo_root>\build-windows\Release" on a local drive.
.PARAMETER QtDir
    Qt 6 prefix directory (e.g. C:\Qt\6.8.3\mingw_64). Automatically detected if omitted.
.PARAMETER NoBuild
    Skip compilation and proceed directly with dependency deployment and installer packaging.
.PARAMETER SkipDeploy
    Skip windeployqt (if dependencies are already deployed in BuildDir).
.PARAMETER Clean
    Wipe the BuildDir before compiling.
.EXAMPLE
    # Auto-detect everything and build
    .\installer\build_exe.ps1

    # Use existing compiled binary in C:\LookAwayBuildVM
    .\installer\build_exe.ps1 -NoBuild

    # Specify custom build directory
    .\installer\build_exe.ps1 -BuildDir "C:\LookAwayBuildVM"
#>

[CmdletBinding()]
param (
    [string]$BuildDir = "",
    [string]$QtDir = "",
    [Alias("v")]
    [string]$Version = "",
    [switch]$NoBuild,
    [switch]$SkipDeploy,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

# Determine directories
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir
if ($RepoRoot.StartsWith("\\") -and ($PWD.Path -notlike "\\*") -and (Test-Path (Join-Path $PWD.Path "CMakeLists.txt"))) {
    $RepoRoot = $PWD.Path
}

# Determine version (auto-extract from CMakeLists.txt if not specified)
if (-not $Version) {
    $cmakeFile = Join-Path $RepoRoot "CMakeLists.txt"
    if (Test-Path $cmakeFile) {
        $cmakeText = Get-Content $cmakeFile -Raw
        if ($cmakeText -match 'project\s*\(\s*LookAway\s+VERSION\s+([0-9\.]+)') {
            $Version = $matches[1]
        }
    }
}
if (-not $Version) {
    $Version = "2.0.0"
}

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  LookAway Windows Packaging & Build Script" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "[+] Repository Root: $RepoRoot"
Write-Host "[+] Target Version:  $Version"

# 1. Environment & Tool Discovery
Write-Host "`n[*] Discovering build tools and environment..." -ForegroundColor Yellow

# Locate Qt installation
if (-not $QtDir) {
    $qtSearchPaths = @(
        "C:\Qt\6.8.3\mingw_64",
        "C:\Qt\6.8.2\mingw_64",
        "C:\Qt\6.8.1\mingw_64",
        "C:\Qt\6.8.0\mingw_64",
        "C:\Qt\6.7.3\mingw_64",
        "C:\Qt\6.7.2\mingw_64"
    )
    foreach ($p in $qtSearchPaths) {
        if (Test-Path "$p\bin\qmake.exe") {
            $QtDir = $p
            break
        }
    }
}

# Locate Inno Setup compiler (ISCC.exe)
$isccLocations = @(
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe",
    "C:\Users\$env:USERNAME\AppData\Local\Programs\Inno Setup 6\ISCC.exe"
)
$isccPath = ""
foreach ($cand in $isccLocations) {
    if (Test-Path $cand) {
        $isccPath = $cand
        break
    }
}
if (-not $isccPath) {
    $cmd = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue
    if ($cmd) {
        $isccPath = $cmd.Source
    }
}

# Add standard tools to PATH
$toolDirs = @(
    "$QtDir\bin",
    "C:\Qt\Tools\mingw1310_64\bin",
    "C:\Qt\Tools\Ninja",
    "C:\Qt\Tools\CMake_64\bin"
)
if ($isccPath) {
    $toolDirs += (Split-Path -Parent $isccPath)
}

foreach ($td in $toolDirs) {
    if ($td -and (Test-Path $td)) {
        if ($env:PATH.IndexOf($td) -eq -1) {
            $env:PATH = "$td;$env:PATH"
        }
    }
}

# Verify required executables
function Find-Tool ($toolName) {
    $t = Get-Command $toolName -ErrorAction SilentlyContinue
    if ($t) { return $t.Source }
    return $null
}

$cmakeExe = Find-Tool "cmake.exe"
$ninjaExe = Find-Tool "ninja.exe"
$windeployqtExe = Find-Tool "windeployqt.exe"

if (-not $cmakeExe) {
    Write-Error "CMake (cmake.exe) was not found in PATH or standard Qt locations!"
}
if (-not $ninjaExe) {
    Write-Warning "Ninja (ninja.exe) was not found in PATH! Make sure Ninja is installed under C:\Qt\Tools\Ninja."
}
if (-not $windeployqtExe) {
    Write-Error "windeployqt.exe was not found! Ensure Qt bin directory exists and is in PATH."
}
if (-not $isccPath) {
    Write-Error "Inno Setup compiler (ISCC.exe) was not found! Install Inno Setup 6 from jrsoftware.org."
}

Write-Host "  -> CMake:       $cmakeExe" -ForegroundColor Gray
Write-Host "  -> Qt Prefix:   $QtDir" -ForegroundColor Gray
Write-Host "  -> Inno Setup:  $isccPath" -ForegroundColor Gray

# 2. Select Build Directory
if (-not $BuildDir) {
    $isNetworkDrive = $false
    if ($RepoRoot.StartsWith("\\")) {
        $isNetworkDrive = $true
    } elseif ($RepoRoot.Length -ge 2 -and $RepoRoot[1] -eq ":") {
        $dl = $RepoRoot.Substring(0, 1)
        $psDrive = Get-PSDrive -Name $dl -ErrorAction SilentlyContinue
        if ($psDrive -and $psDrive.DisplayRoot) {
            $isNetworkDrive = $true
        }
    }

    # If running from network share or if C:\LookAwayBuildVM exists, default to local VM SSD
    if ($isNetworkDrive -or (Test-Path "C:\LookAwayBuildVM")) {
        $BuildDir = "C:\LookAwayBuildVM"
        Write-Host "[i] Detected Virtual Machine / Network Shared Folder ($RepoRoot)." -ForegroundColor Magenta
        Write-Host "    Defaulting BuildDir to fast local SSD drive: $BuildDir" -ForegroundColor Magenta
    } else {
        $BuildDir = Join-Path $RepoRoot "build-windows\Release"
    }
}

Write-Host "[+] Target Build Directory: $BuildDir" -ForegroundColor Green

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "[*] Cleaning build directory: $BuildDir" -ForegroundColor Yellow
    Remove-Item -Path $BuildDir -Recurse -Force
}

# Auto-heal stale CMakeCache.txt if paths changed
$cacheFile = Join-Path $BuildDir "CMakeCache.txt"
if (Test-Path $cacheFile) {
    $cacheText = Get-Content $cacheFile -Raw -ErrorAction SilentlyContinue
    if ($cacheText -match 'CMAKE_CACHEFILE_DIR:INTERNAL=(.+)') {
        $cachedDir = $matches[1].Trim().Replace('/', '\')
        if ($cachedDir -ne $BuildDir.Replace('/', '\')) {
            Write-Warning "Detected stale CMakeCache.txt from a different path ($cachedDir). Resetting cache..."
            Remove-Item $cacheFile -Force -ErrorAction SilentlyContinue
            $cmakeFiles = Join-Path $BuildDir "CMakeFiles"
            if (Test-Path $cmakeFiles) {
                Remove-Item $cmakeFiles -Recurse -Force -ErrorAction SilentlyContinue
            }
        }
    }
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

$exePath = Join-Path $BuildDir "LookAway.exe"

# 3. Compilation Step
if ($NoBuild) {
    Write-Host "`n[*] Skipping build step (-NoBuild specified)..." -ForegroundColor Yellow
    if (-not (Test-Path $exePath)) {
        Write-Error "LookAway.exe was not found at $exePath! Cannot skip build."
    }
} else {
    Write-Host "`n[*] Step 1/3: Configuring and Compiling LookAway (Release)..." -ForegroundColor Yellow

    $cmakeArgs = @(
        "-S", "$RepoRoot",
        "-B", "$BuildDir",
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release"
    )
    if ($QtDir) {
        $cmakeArgs += "-DCMAKE_PREFIX_PATH=$QtDir"
    }

    Write-Host "Running: cmake $cmakeArgs" -ForegroundColor Gray
    & $cmakeExe @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed!"
    }

    Write-Host "Running: cmake --build `"$BuildDir`" --config Release" -ForegroundColor Gray
    & $cmakeExe --build "$BuildDir" --config Release
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Compilation failed!"
    }

    Write-Host "[OK] Executable compiled successfully: $exePath" -ForegroundColor Green
}

# 4. Deploy Qt Runtime Dependencies
if ($SkipDeploy) {
    Write-Host "`n[*] Skipping Qt deployment (-SkipDeploy specified)..." -ForegroundColor Yellow
} else {
    Write-Host "`n[*] Step 2/3: Deploying Qt runtime dependencies via windeployqt..." -ForegroundColor Yellow
    Write-Host "Running: windeployqt `"$exePath`" --compiler-runtime --no-translations" -ForegroundColor Gray
    & $windeployqtExe "$exePath" --compiler-runtime --no-translations
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "windeployqt returned exit code $LASTEXITCODE. Continuing..."
    } else {
        Write-Host "[OK] Dependencies deployed successfully into $BuildDir" -ForegroundColor Green
    }
}

# 5. Build Windows Installer via Inno Setup
Write-Host "`n[*] Step 3/3: Compiling Windows Installer (Inno Setup)..." -ForegroundColor Yellow
$issScript = Join-Path $ScriptDir "build_exe.iss"
$outputDir = Join-Path $RepoRoot "installer_output\windows"

if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}

Write-Host "Running: ISCC.exe /DSourceDir=`"$BuildDir`" /DOutputDir=`"$outputDir`" /DMyAppVersion=`"$Version`" `"$issScript`"" -ForegroundColor Gray
& $isccPath "/DSourceDir=$BuildDir" "/DOutputDir=$outputDir" "/DMyAppVersion=$Version" "$issScript"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Inno Setup compilation failed!"
}

$installerExe = Join-Path $outputDir "LookAway-Setup-v$Version.exe"
if (Test-Path $installerExe) {
    $sizeMB = [math]::Round((Get-Item $installerExe).Length / 1MB, 2)
    Write-Host "`n==========================================================" -ForegroundColor Green
    Write-Host "  BUILD SUCCESSFUL!" -ForegroundColor Green
    Write-Host "  Installer: $installerExe ($sizeMB MB)" -ForegroundColor Green
    Write-Host "==========================================================" -ForegroundColor Green
} else {
    Write-Warning "Installer process completed, but $installerExe was not found."
}
