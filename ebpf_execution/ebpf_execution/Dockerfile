FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    llvm \
    libelf-dev \
    linux-headers-generic \
    libbpf-dev \
    bpftool \
    libncurses5-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY . .

RUN make clean && make all

CMD ["/bin/bash"]
