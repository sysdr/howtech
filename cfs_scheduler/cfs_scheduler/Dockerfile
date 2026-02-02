FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    libncurses5-dev \
    libncurses-dev \
    strace \
    linux-tools-common \
    linux-tools-generic \
    procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /demo
COPY . .

RUN make clean && make

CMD ["/bin/bash"]
