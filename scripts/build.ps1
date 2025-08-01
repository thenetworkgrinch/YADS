param(
    [string]$BuildType = "Release",
    [string]$BuildDir = "build",
    [int]$ParallelJobs = $env:NUMBER_OF_PROCESSORS
)

# Colors for output
$Red = "Red"
$Green = "Green"
$Yellow = "Yellow"
$Blue = "Blue"

Write-Host "=== Yet Another Driver Station Build Script ===" -ForegroundColor $Blue
Write-Host "Build Type: $BuildType" -ForegroundColor $Blue
Write-Host "Parallel Jobs: $ParallelJobs" -ForegroundColor $Blue
Write-Host ""

# Check dependencies
Write-Host "Checking dependencies..." -ForegroundColor $Yellow

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host "Error: CMake is not installed" -ForegroundColor $Red
    exit 1
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "Error: Git is not installed" -ForegroundColor $Red
    exit 1
}

$cmakeVersion = (cmake --version | Select-Object -First 1).Split(' ')[2]
Write-Host "CMake version: $cmakeVersion" -ForegroundColor $Green

# Check Qt installation
if (-not (Get-Command qmake -ErrorAction SilentlyContinue)) {
    Write-Host "Error: Qt is not installed or not in PATH" -ForegroundColor $Red
    Write-Host "Please install Qt 6.5.0 or later" -ForegroundColor $Yellow
    exit 1
}

$qtVersion = qmake -query QT_VERSION
Write-Host "Qt version: $qtVersion" -ForegroundColor $Green

# Clone QHotkey if not present
if (-not (Test-Path "thirdparty/QHotkey")) {
    Write-Host "Cloning QHotkey..." -ForegroundColor $Yellow
    New-Item -ItemType Directory -Force -Path "thirdparty" | Out-Null
    git clone --depth 1 https://github.com/Skycoder42/QHotkey.git thirdparty/QHotkey
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to clone QHotkey" -ForegroundColor $Red
        exit 1
    }
    Write-Host "QHotkey cloned successfully" -ForegroundColor $Green
} else {
    Write-Host "QHotkey already present" -ForegroundColor $Green
}

# Create build directory
Write-Host "Setting up build directory..." -ForegroundColor $Yellow
if (Test-Path $BuildDir) {
    Write-Host "Cleaning existing build directory..." -ForegroundColor $Yellow
    Remove-Item -Recurse -Force $BuildDir
}
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

# Configure with CMake
Write-Host "Configuring with CMake..." -ForegroundColor $Yellow
Set-Location $BuildDir

$cmakeArgs = @(
    ".."
    "-DCMAKE_BUILD_TYPE=$BuildType"
    "-DENABLE_GLOBAL_SHORTCUTS=ON"
    "-DENABLE_FMS_SUPPORT=ON"
    "-DENABLE_GLASS_INTEGRATION=ON"
    "-DENABLE_DASHBOARD_MANAGEMENT=ON"
    "-DENABLE_PRACTICE_MATCH=ON"
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
)

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed" -ForegroundColor $Red
    Set-Location ..
    exit 1
}

Write-Host "CMake configuration successful" -ForegroundColor $Green

# Build
Write-Host "Building application..." -ForegroundColor $Yellow
cmake --build . --config $BuildType --parallel $ParallelJobs

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed" -ForegroundColor $Red
    Set-Location ..
    exit 1
}

Write-Host "Build successful!" -ForegroundColor $Green

# Show build results
Write-Host ""
Write-Host "=== Build Complete ===" -ForegroundColor $Blue
Write-Host "Executable location:" -ForegroundColor $Green

$executablePath = if ($BuildType -eq "Debug") {
    ".\Debug\YetAnotherDriverStation.exe"
} else {
    ".\Release\YetAnotherDriverStation.exe"
}

if (Test-Path $executablePath) {
    $fullPath = Resolve-Path $executablePath
    Write-Host "  $fullPath" -ForegroundColor $Green
    
    # Show file size
    $fileSize = (Get-Item $executablePath).Length
    $fileSizeMB = [math]::Round($fileSize / 1MB, 2)
    Write-Host "  Size: $fileSizeMB MB" -ForegroundColor $Green
} else {
    Write-Host "  Executable not found at expected location" -ForegroundColor $Red
    Set-Location ..
    exit 1
}

Write-Host ""
Write-Host "To run the application:" -ForegroundColor $Blue
Write-Host "  cd $BuildDir && $executablePath" -ForegroundColor $Green
Write-Host ""
Write-Host "To install system-wide:" -ForegroundColor $Blue
Write-Host "  cmake --install . --config $BuildType" -ForegroundColor $Green
Write-Host ""

Set-Location ..
Write-Host "Build script completed successfully!" -ForegroundColor $Green
