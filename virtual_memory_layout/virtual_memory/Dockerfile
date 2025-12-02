FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    strace \
    valgrind \
    linux-tools-generic \
    libncurses5-dev \
    procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY . .

RUN make

CMD ["/bin/bash"]
