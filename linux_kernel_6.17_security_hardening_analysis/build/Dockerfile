FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && apt-get install -y --no-install-recommends \
    build-essential \
    libncurses-dev \
    strace \
    ltrace \
    linux-tools-common \
    linux-tools-generic \
    valgrind \
    libseccomp-dev \
    libseccomp2 \
    seccomp \
    gdb \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /demo
COPY src/ ./src/
COPY Makefile .
RUN make all
CMD ["/bin/bash"]
