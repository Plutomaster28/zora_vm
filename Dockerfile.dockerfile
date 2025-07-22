# Create Dockerfile in your project root
# Multi-stage build for optimal size and security
FROM ubuntu:22.04 AS builder

# Set environment to avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    # Lua dependencies
    liblua5.4-dev \
    lua5.4 \
    # Python dependencies  
    python3-dev \
    python3 \
    # Perl dependencies
    libperl-dev \
    perl \
    # Networking dependencies
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Create build directory
WORKDIR /build

# Copy source code
COPY . .

# Build Zora VM with maximum optimizations
RUN echo "Building Zora VM with Meisei Virtual Silicon..." && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DENABLE_MEISEI_PGO=OFF \
          -DCMAKE_C_FLAGS="-O3 -march=x86-64 -mtune=generic" \
          . && \
    ninja -v

# Verify the build worked
RUN echo "Build verification..." && \
    ls -la zora_vm* && \
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
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# Create zora user for security (don't run as root)
RUN useradd -m -s /bin/bash -u 1000 zora && \
    echo "zora ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Copy the built VM from builder stage
COPY --from=builder /build/zora_vm /usr/local/bin/zora_vm
COPY --from=builder /build/include /usr/local/include/zora/

# Create VM directory structure
RUN mkdir -p /home/zora/ZoraPerl/{documents,scripts,data,projects} && \
    mkdir -p /home/zora/.zora/{config,cache,logs} && \
    chown -R zora:zora /home/zora

# Set up sample files for demo
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
    echo 'echo "  network show interfaces"' >> /home/zora/welcome.sh && \
    echo 'echo "  help"' >> /home/zora/welcome.sh && \
    echo 'echo ""' >> /home/zora/welcome.sh && \
    chmod +x /home/zora/welcome.sh

# Health check to verify VM is working
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD echo "help" | timeout 5 zora_vm --batch-mode 2>/dev/null || exit 1

# Labels for metadata
LABEL maintainer="theni" \
      description="Zora VM - Advanced Virtual Machine with Meisei Virtual Silicon" \
      version="1.0" \
      architecture="x86_64"

# Default command - start Zora VM
CMD ["/bin/bash", "-c", "/home/zora/welcome.sh && exec zora_vm"]