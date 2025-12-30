FROM ubuntu:22.04

# Install required packages
RUN apt-get update && apt-get install -y \
    build-essential \
    gdb \
    python3 \
    strace \
    file \
    binutils \
    && rm -rf /var/lib/apt/lists/*

# Set core dump pattern
RUN mkdir -p /cores && chmod 777 /cores

WORKDIR /app

# Copy source files
COPY src/ ./src/
COPY Makefile ./

# Build
RUN make

CMD ["/bin/bash"]
