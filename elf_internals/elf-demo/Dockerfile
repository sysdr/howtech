FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    binutils \
    gdb \
    strace \
    ltrace \
    elfutils \
    file \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . /workspace/

RUN make all && make plt_monitor.so

CMD ["/bin/bash"]
