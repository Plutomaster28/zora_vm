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

# Set environment
ENV DEBIAN_FRONTEND=noninteractive

# Install only runtime dependencies (much smaller)
RUN apt-get update && apt-get install -y \
    # Lua runtime
    liblua5.4-0 \
    lua5.4 \
    # Python runtime
    python3 \
    python3-minimal \
    # Perl runtime
    perl \
    # SSL for networking
    libssl3 \
    # Basic utilities
    curl \
    wget \
    nano \
    # Additional runtime libraries
    libreadline8 \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# Create zora user for security (don't run as root)
RUN useradd -m -s /bin/bash -u 1000 zora

# Copy the built VM from builder stage
COPY --from=builder /build/zora_vm /usr/local/bin/zora_vm

# Make sure it's executable
RUN chmod +x /usr/local/bin/zora_vm

# Create VM directory structure (fresh every time)
RUN mkdir -p /home/zora/ZoraPerl/{documents,scripts,data,projects} && \
    mkdir -p /home/zora/.zora/{config,cache,logs} && \
    chown -R zora:zora /home/zora

# Set up sample files for demo (will be overridden by volumes if mounted)
RUN echo 'print("Hello from Zora VM in Docker!")' > /home/zora/ZoraPerl/scripts/hello.lua && \
    echo 'print("Python works too!")' > /home/zora/ZoraPerl/scripts/hello.py && \
    echo 'print "Perl is ready!\n";' > /home/zora/ZoraPerl/scripts/hello.pl && \
    chown -R zora:zora /home/zora/ZoraPerl

# Switch to zora user
USER zora
WORKDIR /home/zora

# Set up environment variables
ENV ZORA_VM_HOME=/home/zora
ENV ZORA_PERL_PATH=/home/zora/ZoraPerl
ENV PATH="/usr/local/bin:$PATH"

# Create a welcome script
RUN echo '#!/bin/bash' > /home/zora/welcome.sh && \
    echo 'echo "Welcome to Zora VM in Docker!"' >> /home/zora/welcome.sh && \
    echo 'echo "Your persistent files are in ~/ZoraPerl/"' >> /home/zora/welcome.sh && \
    echo 'echo "Try these commands:"' >> /home/zora/welcome.sh && \
    echo 'echo "  lua ZoraPerl/scripts/hello.lua"' >> /home/zora/welcome.sh && \
    echo 'echo "  python ZoraPerl/scripts/hello.py"' >> /home/zora/welcome.sh && \
    echo 'echo "  perl ZoraPerl/scripts/hello.pl"' >> /home/zora/welcome.sh && \
    echo 'echo "  help"' >> /home/zora/welcome.sh && \
    echo 'echo ""' >> /home/zora/welcome.sh && \
    chmod +x /home/zora/welcome.sh

# Labels for metadata
LABEL maintainer="plutomaster28" \
      description="Zora VM - Advanced Virtual Machine with Meisei Virtual Silicon" \
      version="1.0"

# Default command - start Zora VM
CMD ["/bin/bash", "-c", "/home/zora/welcome.sh && exec zora_vm"]