FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    valgrind \
    strace \
    linux-tools-common \
    libncurses-dev \
    time \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY src/ ./src/
COPY Makefile .
RUN make all
