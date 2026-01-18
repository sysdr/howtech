FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    mingw-w64 \
    g++-mingw-w64 \
    binutils-mingw-w64 \
    cmake \
    make \
    wget \
    vim \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Copy source files
COPY src/ /workspace/src/

# Build user-mode tool (cross-compile for Windows)
WORKDIR /workspace/src/usermode
RUN make

WORKDIR /workspace

CMD ["/bin/bash"]
