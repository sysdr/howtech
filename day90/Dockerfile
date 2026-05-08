FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential make gcc libc6-dev \
    libncurses6 libncurses-dev \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY src/ /build/
RUN make -s

CMD ["./dsq_sim"]
