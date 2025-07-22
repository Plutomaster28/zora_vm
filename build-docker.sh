#!/bin/bash
echo "Building Zora VM Docker image..."

# Build the image (lowercase name)
docker build -t ghcr.io/plutomaster28/zora-vm:latest .

if [ $? -eq 0 ]; then
    echo "Docker image built successfully!"
    echo ""
    echo "To run Zora VM:"
    echo "   docker run -it ghcr.io/plutomaster28/zora-vm:latest"
    echo ""
    echo "To run with persistent storage:"
    echo "   docker run -it -v \$(pwd)/my-data:/home/zora/ZoraPerl ghcr.io/plutomaster28/zora-vm:latest"
    echo ""
    echo "To run with network access:"
    echo "   docker run -it --network host ghcr.io/plutomaster28/zora-vm:latest"
else
    echo "Docker build failed!"
    exit 1
fi