param(
    [string]$BuildType = "Release",
    [switch]$Clean
)

Write-Host "Building Yet Another Driver Station..." -ForegroundColor Green

# Clean build directory if requested
if ($Clean -and (Test-Path "build")) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build"
}

# Create build directory
if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Set-Location "build"

try {
    # Clone QHotkey if not present
    if (!(Test-Path "../thirdparty/QHotkey")) {
        Write-Host "Cloning QHotkey..." -ForegroundColor Yellow
        if (!(Test-Path "../thirdparty")) {
            New-Item -ItemType Directory -Path "../thirdparty" | Out-Null
        }
        git clone https://github.com/Skycoder42/QHotkey.git ../thirdparty/QHotkey
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to clone QHotkey"
        }
    }

    # Configure with CMake
    Write-Host "Configuring with CMake..." -ForegroundColor Yellow
    cmake .. `
        -DCMAKE_BUILD_TYPE=$BuildType `
        -DENABLE_GLOBAL_SHORTCUTS=ON `
        -DENABLE_FMS_SUPPORT=ON `
        -DENABLE_GLASS_INTEGRATION=ON `
        -DENABLE_DASHBOARD_MANAGEMENT=ON `
        -DENABLE_PRACTICE_MATCH=ON

    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }

    # Build
    Write-Host "Building..." -ForegroundColor Yellow
    cmake --build . --config $BuildType --parallel

    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }

    Write-Host "Build completed successfully!" -ForegroundColor Green
    Write-Host "Executable location: build/$BuildType/YetAnotherDriverStation.exe" -ForegroundColor Cyan

} catch {
    Write-Host "Build failed: $_" -ForegroundColor Red
    exit 1
} finally {
    Set-Location ".."
}
