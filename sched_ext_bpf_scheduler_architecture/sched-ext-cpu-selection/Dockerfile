FROM ubuntu:24.04

# Install required packages
RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    llvm \
    libbpf-dev \
    libelf-dev \
    linux-headers-generic \
    linux-tools-generic \
    linux-tools-common \
    libncurses-dev \
    zlib1g-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/* \
    && ln -s /usr/lib/linux-tools/*/bpftool /usr/local/bin/bpftool || true

WORKDIR /workspace

# Copy source files
COPY . /workspace/

# Build everything
RUN make clean && make

CMD ["/bin/bash"]
