# Create Dockerfile in your project root
# Multi-stage build for optimal size and security
FROM ubuntu:22.04 AS builder

# Set environment to avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
ENV DOCKER_BUILD=1

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential cmake ninja-build \
    liblua5.4-dev lua5.4 \
    python3-dev python3 \
    libperl-dev perl \
    libssl-dev pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Create build directory
WORKDIR /build

# Copy source code
COPY . .

# Debug: Show what we have and find libraries
RUN echo "=== FILES COPIED ===" && \
    ls -la && \
    echo "=== SOURCE STRUCTURE ===" && \
    find src -name "*.c" | head -5 2>/dev/null || echo "No src/*.c found" && \
    echo "=== CMAKE EXISTS ===" && \
    ls -la CMakeLists.txt

# Configure with verbose output
RUN echo "Configuring Zora VM..." && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_VERBOSE_MAKEFILE=ON \
          -DCMAKE_C_FLAGS="-O2" \
          . 2>&1 | tee cmake_output.log || \
    (echo "CMake configuration failed!" && \
     echo "CMake output:" && cat cmake_output.log && \
     echo "CMake error log:" && cat CMakeFiles/CMakeError.log 2>/dev/null && \
     echo "CMake output log:" && cat CMakeFiles/CMakeOutput.log 2>/dev/null && \
     exit 1)

# Build with verbose output
RUN echo "Building Zora VM..." && \
    ninja -v 2>&1 | tee build_output.log || \
    (echo "Build failed!" && \
     echo "Build output:" && cat build_output.log && \
     echo "Last 50 lines of build:" && tail -50 build_output.log && \
     exit 1)

# Verify the build worked
RUN echo "Build verification..." && \
    ls -la zora_vm* && \
    file zora_vm && \
    ldd zora_vm || true

# Runtime stage - minimal image for distribution
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential cmake ninja-build \
    liblua5.4-dev lua5.4 python3-dev python3 libperl-dev perl \
    libssl-dev pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

# Debug what we have
RUN echo "=== FILES COPIED ===" && \
    ls -la && \
    echo "=== SOURCE STRUCTURE ===" && \
    find src -name "*.c" | head -5 2>/dev/null || echo "No src/*.c found" && \
    echo "=== CMAKE EXISTS ===" && \
    ls -la CMakeLists.txt

# Try to build and see what happens
RUN echo "=== CONFIGURING ===" && \
    cmake . && \
    echo "=== BUILDING ===" && \
    ninja && \
    echo "=== WHAT GOT BUILT ===" && \
    ls -la && \
    find . -name "zora_vm*" && \
    echo "=== FILE TYPES ===" && \
    file zora_vm* 2>/dev/null || echo "No zora_vm files found"

# If we get here, we have a Linux binary!
CMD ["./zora_vm"]