FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    libncurses5-dev \
    libncursesw5-dev \
    openocd \
    gdb-multiarch \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . /app/

RUN make clean && make all

CMD ["/bin/bash"]
