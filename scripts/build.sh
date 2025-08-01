#!/bin/bash

set -e

echo "Building Yet Another Driver Station..."

# Create build directory
mkdir -p build
cd build

# Clone QHotkey if not present
if [ ! -d "../thirdparty/QHotkey" ]; then
    echo "Cloning QHotkey..."
    mkdir -p ../thirdparty
    git clone https://github.com/Skycoder42/QHotkey.git ../thirdparty/QHotkey
fi

# Configure with CMake
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_GLOBAL_SHORTCUTS=ON \
    -DENABLE_FMS_SUPPORT=ON \
    -DENABLE_GLASS_INTEGRATION=ON \
    -DENABLE_DASHBOARD_MANAGEMENT=ON \
    -DENABLE_PRACTICE_MATCH=ON

# Build
echo "Building..."
cmake --build . --config Release --parallel $(nproc)

echo "Build completed successfully!"
echo "Executable location: build/YetAnotherDriverStation"
