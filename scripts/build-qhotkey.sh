#!/bin/bash
set -e

echo "Building QHotkey for Yet Another Driver Station"

# Create thirdparty directory if it doesn't exist
mkdir -p thirdparty

# Clone QHotkey if not already present
if [ ! -d "thirdparty/QHotkey" ]; then
    echo "Cloning QHotkey..."
    git clone https://github.com/Skycoder42/QHotkey.git thirdparty/QHotkey
    cd thirdparty/QHotkey
    git checkout master
    cd ../..
else
    echo "QHotkey already exists, updating..."
    cd thirdparty/QHotkey
    git pull
    cd ../..
fi

# Build QHotkey
echo "Building QHotkey..."
cd thirdparty/QHotkey

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring QHotkey with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install

# Build
echo "Compiling QHotkey..."
make -j$(nproc)

# Install to local directory
echo "Installing QHotkey..."
make install

echo "QHotkey build completed successfully!"
echo "Library installed to: $(pwd)/install"

cd ../../..
