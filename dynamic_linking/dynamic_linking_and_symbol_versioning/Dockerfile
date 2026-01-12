FROM ubuntu:22.04

# Install build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    binutils \
    gdb \
    strace \
    ltrace \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Copy source files
COPY src/ /workspace/src/
COPY build.sh /workspace/

# Build everything
RUN chmod +x build.sh && ./build.sh

CMD ["/bin/bash"]
