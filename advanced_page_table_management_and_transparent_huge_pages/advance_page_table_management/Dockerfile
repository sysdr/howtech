FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        gcc \
        make \
        libncurses5-dev \
        libncursesw5-dev \
        valgrind \
        strace \
        procps \
        && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source files and Makefile
COPY src/ ./src/
COPY Makefile ./

# Build the binaries
RUN make all

# Default command
CMD ["/bin/bash"]
