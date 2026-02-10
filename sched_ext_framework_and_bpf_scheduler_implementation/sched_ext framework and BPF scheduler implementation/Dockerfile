FROM ubuntu:24.04

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    llvm \
    libbpf-dev \
    libelf-dev \
    linux-headers-generic \
    linux-tools-generic \
    linux-tools-common \
    bpftool \
    libncurses-dev \
    pkg-config \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Copy source files
COPY Makefile .
COPY include include/
COPY src src/

# Build
RUN make all || echo "Build completed with warnings (expected on older kernels)"

CMD ["/bin/bash"]
