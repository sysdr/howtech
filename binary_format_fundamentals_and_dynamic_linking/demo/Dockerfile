FROM ubuntu:24.04

# Install build tools and analysis utilities
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    gdb \
    strace \
    ltrace \
    binutils \
    elfutils \
    file \
    vim \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Copy demo files
COPY . .

# Build everything
RUN make clean && make

CMD ["/bin/bash"]
