FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    gdb \
    ltrace \
    strace \
    binutils \
    elfutils \
    ncurses-dev \
    file \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY src/ ./src/

RUN cd src && make clean && make all

ENV LD_LIBRARY_PATH=/workspace/build:$LD_LIBRARY_PATH

CMD ["/bin/bash"]
