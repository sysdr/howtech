FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    llvm \
    libbpf-dev \
    linux-headers-generic \
    linux-tools-common \
    linux-tools-generic \
    libelf-dev \
    zlib1g-dev \
    libncurses-dev \
    curl \
    iproute2 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY . .

RUN make clean && make

CMD ["/bin/bash"]
