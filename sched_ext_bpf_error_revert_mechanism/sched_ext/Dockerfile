FROM ubuntu:24.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      build-essential pkg-config libncurses-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY src/ /build/

RUN make -C /build all

ENTRYPOINT ["/bin/bash", "-lc"]
