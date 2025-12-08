FROM ubuntu:22.04

# Install build essentials and debugging tools
RUN apt-get update && apt-get install -y \
    build-essential \
    gdb \
    strace \
    ltrace \
    valgrind \
    linux-tools-generic \
    ncurses-dev \
    procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /demo

# Copy source files
COPY . .

# Build everything
RUN make clean && make

CMD ["/bin/bash"]
