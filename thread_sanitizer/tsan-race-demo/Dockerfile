FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    gcc \
    g++ \
    gdb \
    strace \
    valgrind \
    linux-tools-generic \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY . .

RUN chmod +x *.sh 2>/dev/null || true

CMD ["/bin/bash"]
