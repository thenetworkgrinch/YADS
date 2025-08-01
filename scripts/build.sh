#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_TYPE=${1:-Release}
BUILD_DIR="build"
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo -e "${BLUE}=== Yet Another Driver Station Build Script ===${NC}"
echo -e "${BLUE}Build Type: ${BUILD_TYPE}${NC}"
echo -e "${BLUE}Parallel Jobs: ${PARALLEL_JOBS}${NC}"
echo

# Check dependencies
echo -e "${YELLOW}Checking dependencies...${NC}"

if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: CMake is not installed${NC}"
    exit 1
fi

if ! command -v git &> /dev/null; then
    echo -e "${RED}Error: Git is not installed${NC}"
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
echo -e "${GREEN}CMake version: ${CMAKE_VERSION}${NC}"

# Check Qt installation
if ! command -v qmake &> /dev/null; then
    echo -e "${RED}Error: Qt is not installed or not in PATH${NC}"
    echo -e "${YELLOW}Please install Qt 6.5.0 or later${NC}"
    exit 1
fi

QT_VERSION=$(qmake -query QT_VERSION)
echo -e "${GREEN}Qt version: ${QT_VERSION}${NC}"

# Clone QHotkey if not present
if [ ! -d "thirdparty/QHotkey" ]; then
    echo -e "${YELLOW}Cloning QHotkey...${NC}"
    mkdir -p thirdparty
    git clone --depth 1 https://github.com/Skycoder42/QHotkey.git thirdparty/QHotkey
    echo -e "${GREEN}QHotkey cloned successfully${NC}"
else
    echo -e "${GREEN}QHotkey already present${NC}"
fi

# Create build directory
echo -e "${YELLOW}Setting up build directory...${NC}"
if [ -d "${BUILD_DIR}" ]; then
    echo -e "${YELLOW}Cleaning existing build directory...${NC}"
    rm -rf "${BUILD_DIR}"
fi
mkdir -p "${BUILD_DIR}"

# Configure with CMake
echo -e "${YELLOW}Configuring with CMake...${NC}"
cd "${BUILD_DIR}"

cmake .. \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DENABLE_GLOBAL_SHORTCUTS=ON \
    -DENABLE_FMS_SUPPORT=ON \
    -DENABLE_GLASS_INTEGRATION=ON \
    -DENABLE_DASHBOARD_MANAGEMENT=ON \
    -DENABLE_PRACTICE_MATCH=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed${NC}"
    exit 1
fi

echo -e "${GREEN}CMake configuration successful${NC}"

# Build
echo -e "${YELLOW}Building application...${NC}"
cmake --build . --config "${BUILD_TYPE}" --parallel "${PARALLEL_JOBS}"

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"

# Show build results
echo
echo -e "${BLUE}=== Build Complete ===${NC}"
echo -e "${GREEN}Executable location:${NC}"

if [ "${BUILD_TYPE}" = "Debug" ]; then
    EXECUTABLE_PATH="./YetAnotherDriverStation"
else
    EXECUTABLE_PATH="./YetAnotherDriverStation"
fi

if [ -f "${EXECUTABLE_PATH}" ]; then
    echo -e "${GREEN}  $(pwd)/${EXECUTABLE_PATH}${NC}"
    
    # Show file size
    FILE_SIZE=$(du -h "${EXECUTABLE_PATH}" | cut -f1)
    echo -e "${GREEN}  Size: ${FILE_SIZE}${NC}"
    
    # Show dependencies (Linux only)
    if command -v ldd &> /dev/null; then
        echo -e "${YELLOW}Dependencies check:${NC}"
        if ldd "${EXECUTABLE_PATH}" | grep -q "not found"; then
            echo -e "${RED}  Warning: Missing dependencies detected${NC}"
            ldd "${EXECUTABLE_PATH}" | grep "not found"
        else
            echo -e "${GREEN}  All dependencies satisfied${NC}"
        fi
    fi
else
    echo -e "${RED}  Executable not found at expected location${NC}"
    exit 1
fi

echo
echo -e "${BLUE}To run the application:${NC}"
echo -e "${GREEN}  cd ${BUILD_DIR} && ${EXECUTABLE_PATH}${NC}"
echo
echo -e "${BLUE}To install system-wide:${NC}"
echo -e "${GREEN}  sudo cmake --install . --config ${BUILD_TYPE}${NC}"
echo

echo -e "${GREEN}Build script completed successfully!${NC}"
