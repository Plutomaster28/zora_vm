# Create Dockerfile in your project root
# Multi-stage build for optimal size and security
FROM ubuntu:22.04 AS builder

# Set environment to avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
ENV DOCKER_BUILD=1
ENV PLATFORM_LINUX=1

# Install build dependencies including seccomp for sandboxing
RUN apt-get update && apt-get install -y \
    build-essential cmake ninja-build git \
    liblua5.4-dev lua5.4 \
    python3-dev python3 \
    libperl-dev perl \
    libssl-dev pkg-config \
    libseccomp-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

# Create build directory and configure
RUN echo "=== CREATING BUILD DIR ===" && \
    mkdir -p build && \
    cd build && \
    echo "=== CONFIGURING CMAKE ===" && \
    cmake -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DLINUX_BUILD=ON \
        -DPLATFORM_LINUX=ON \
        -DCROSS_PLATFORM_SANDBOX=ON \
        -DVIRTUAL_FILESYSTEM=ON \
        -DELF_BINARY_SUPPORT=ON \
        -DLUA_SCRIPTING=ON \
        -DSANDBOXED_SYSCALLS=ON \
        .. && \
    echo "=== BUILDING WITH NINJA ===" && \
    ninja -v && \
    echo "=== VERIFYING BUILD ===" && \
    ls -la . && \
    if [ -f zora_vm ]; then \
        echo "Build successful!"; \
        file zora_vm; \
        ldd zora_vm; \
        echo "=== TESTING EXECUTABLE ==="; \
        ./zora_vm --version || echo "Version test completed"; \
    else \
        echo "Build failed - executable not found!"; \
        echo "Contents of build directory:"; \
        find . -name "*zora*" -o -name "*.exe" 2>/dev/null || true; \
        echo "CMake files:"; \
        ls -la CMakeFiles/ 2>/dev/null || true; \
        exit 1; \
    fi && \
    echo "=== BUILD SUCCESS! ==="

# Runtime stage
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    liblua5.4-0 \
    python3 \
    perl \
    libseccomp2 \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# Copy the built executable
COPY --from=builder /build/build/zora_vm /usr/local/bin/zora_vm
RUN chmod +x /usr/local/bin/zora_vm

# Create a non-root user for additional security
RUN useradd -m -s /bin/bash zora && \
    mkdir -p /home/zora/vm_data /home/zora/persistent && \
    chown -R zora:zora /home/zora

# Create directories for VM operations
RUN mkdir -p /var/lib/zora_vm /tmp/zora_sandbox && \
    chown zora:zora /var/lib/zora_vm /tmp/zora_sandbox

USER zora
WORKDIR /home/zora

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD /usr/local/bin/zora_vm --health || exit 1

# Expose any ports if needed (adjust as necessary)
EXPOSE 8080

# Add some environment variables for the VM
ENV ZORA_VM_HOME=/home/zora
ENV ZORA_VM_DATA=/home/zora/vm_data
ENV ZORA_VM_PERSISTENT=/home/zora/persistent

CMD ["/usr/local/bin/zora_vm"]