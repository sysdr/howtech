FROM ubuntu:22.04

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    linux-headers-generic \
    bc \
    flex \
    bison \
    libelf-dev \
    libssl-dev \
    kmod \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Copy project files
COPY . /workspace/

# Build instructions
RUN echo "To build the module:" && \
    echo "  cd src && make" && \
    echo "" && \
    echo "Note: Module cannot be loaded in Docker without --privileged" && \
    echo "Use the host system or a VM for actual module loading"

CMD ["/bin/bash"]
