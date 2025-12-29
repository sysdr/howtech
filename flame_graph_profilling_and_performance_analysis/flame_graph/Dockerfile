FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    linux-tools-generic \
    linux-tools-common \
    git \
    perl \
    wget \
    procps \
    && rm -rf /var/lib/apt/lists/*

# Install FlameGraph tools
RUN git clone https://github.com/brendangregg/FlameGraph /opt/FlameGraph && \
    chmod +x /opt/FlameGraph/*.pl

ENV PATH="/opt/FlameGraph:${PATH}"

WORKDIR /workspace
