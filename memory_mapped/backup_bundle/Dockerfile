FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    libncurses5-dev \
    strace \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN make clean && make

CMD ["/bin/bash"]
