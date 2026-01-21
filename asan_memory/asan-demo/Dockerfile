FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    clang \
    llvm \
    gdb \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /asan-demo

COPY . .

RUN make asan

CMD ["/bin/bash"]
