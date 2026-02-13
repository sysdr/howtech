FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    linux-headers-generic \
    libncurses-dev \
    valgrind \
    strace \
    linux-tools-generic \
    vim \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . /workspace/

CMD ["/bin/bash"]
