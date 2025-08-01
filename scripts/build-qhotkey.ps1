# Build QHotkey for Yet Another Driver Station
param(
    [string]$Configuration = "Release"
)

Write-Host "Building QHotkey for Yet Another Driver Station" -ForegroundColor Green

# Create thirdparty directory if it doesn't exist
if (!(Test-Path "thirdparty")) {
    New-Item -ItemType Directory -Path "thirdparty"
}

# Clone QHotkey if not already present
if (!(Test-Path "thirdparty/QHotkey")) {
    Write-Host "Cloning QHotkey..." -ForegroundColor Yellow
    git clone https://github.com/Skycoder42/QHotkey.git thirdparty/QHotkey
    Set-Location thirdparty/QHotkey
    git checkout master
    Set-Location ../..
} else {
    Write-Host "QHotkey already exists, updating..." -ForegroundColor Yellow
    Set-Location thirdparty/QHotkey
    git pull
    Set-Location ../..
}

# Build QHotkey
Write-Host "Building QHotkey..." -ForegroundColor Yellow
Set-Location thirdparty/QHotkey

# Create build directory
if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Path "build"
}
Set-Location build

# Configure with CMake
Write-Host "Configuring QHotkey with CMake..." -ForegroundColor Cyan
cmake .. -DCMAKE_BUILD_TYPE=$Configuration -DCMAKE_INSTALL_PREFIX=install -G "Visual Studio 16 2019" -A x64

# Build
Write-Host "Compiling QHotkey..." -ForegroundColor Cyan
cmake --build . --config $Configuration

# Install to local directory
Write-Host "Installing QHotkey..." -ForegroundColor Cyan
cmake --install . --config $Configuration

Write-Host "QHotkey build completed successfully!" -ForegroundColor Green
Write-Host "Library installed to: $(Get-Location)/install" -ForegroundColor Green

Set-Location ../../..
