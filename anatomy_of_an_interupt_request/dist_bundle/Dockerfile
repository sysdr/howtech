FROM ubuntu:22.04

# Install required packages
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    libncurses-dev \
    strace \
    linux-tools-generic \
    procps \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source files
COPY src/ ./src/
COPY Makefile ./

# Build programs
RUN make all

CMD ["/bin/bash"]
