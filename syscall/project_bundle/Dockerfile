FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    strace \
    linux-tools-generic \
    binutils \
    procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN make clean && make

CMD ["/bin/bash"]
