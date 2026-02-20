FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && \
    apt-get install -y -q --no-install-recommends \
        gcc g++ make \
        libncurses-dev \
        valgrind \
        strace \
        procps \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY src/ /app/src/
COPY bin/  /app/bin/ 2>/dev/null || true

RUN mkdir -p /app/bin && \
    cd /app/src && \
    make CC=gcc CFLAGS="-Wall -Wextra -Werror -O2 -D_GNU_SOURCE" all

CMD ["/app/bin/alloc_demo"]
