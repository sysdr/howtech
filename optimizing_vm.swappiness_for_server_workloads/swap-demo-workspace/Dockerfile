FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && \
    apt-get install -y --no-install-recommends \
        build-essential gcc make \
        stress-ng procps iproute2 \
        libncurses-dev linux-headers-generic \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY src/ ./src/
COPY Makefile .
RUN make all
CMD ["/bin/bash"]
