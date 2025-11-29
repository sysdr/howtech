FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    strace \
    linux-tools-generic \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app/

RUN make clean && make

CMD ["/bin/bash"]
