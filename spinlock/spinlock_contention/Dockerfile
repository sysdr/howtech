FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    linux-tools-generic \
    linux-tools-common \
    strace \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY . .

RUN make clean && make

CMD ["/bin/bash"]
