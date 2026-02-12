FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    libncurses5-dev \
    procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY src/ ./

RUN make clean && make all

CMD ["./buddy_monitor"]
