FROM rust:1.75-slim

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    strace \
    procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY Makefile .
COPY src/ ./src/

RUN make all

CMD ["/bin/bash"]
