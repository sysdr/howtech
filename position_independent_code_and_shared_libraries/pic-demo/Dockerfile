FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    binutils \
    file \
    strace \
    ltrace \
    gdb \
    procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . /workspace/

RUN make clean && make all

CMD ["/bin/bash"]
