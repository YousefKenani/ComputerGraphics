#!/bin/bash

# Stop on error
set -e

BUILD_DIR="build"

# Create build folder if it doesn't exist
mkdir -p $BUILD_DIR
cd $BUILD_DIR

echo "🔧 Configuring with CMake..."
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..

echo "⚙️ Building with Ninja..."
ninja

echo "🚀 Running app..."
./minigui

