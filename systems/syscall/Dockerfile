FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    strace \
    linux-tools-generic \
    gdb \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /demo

COPY syscall_demo.c monitor.c Makefile ./

RUN make all

CMD ["./syscall_demo"]
