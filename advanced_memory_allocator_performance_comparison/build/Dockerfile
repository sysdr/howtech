FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && apt-get install -y --no-install-recommends \
    build-essential \
    strace \
    linux-tools-generic \
    valgrind \
    libtcmalloc-minimal4 \
    libgoogle-perftools-dev \
    libjemalloc-dev \
    libgoogle-perftools4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /bench
COPY alloc_bench.c Makefile syscall_trace.sh monitor.sh ./
RUN chmod +x *.sh && make all
CMD ["/bin/bash"]
