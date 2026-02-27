FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && \
    apt-get install -y --no-install-recommends \
        build-essential gcc make strace ltrace \
        procps linux-tools-common \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY Makefile .
COPY src/ src/

RUN make all

CMD ["/bin/bash", "-c", "echo '' && echo -e '\\033[1;36m══════════════════════════════════════════════════════════\\033[0m' && echo -e '\\033[1;36m  ASIL B Kernel Burden Demo — Running All Demonstrations  \\033[0m' && echo -e '\\033[1;36m══════════════════════════════════════════════════════════\\033[0m' && echo '' && echo -e '\\033[1;33m[1/4] Qualification Scope Comparison\\033[0m' && ./qualification_scope && echo -e '\\033[1;33m[2/4] UIO Userspace Driver Simulation\\033[0m' && ./uio_sim && echo -e '\\033[1;33m[3/4] IOMMU / Memory Isolation Inspection\\033[0m' && ./iommu_inspect && echo -e '\\033[1;33m[4/4] Latency Jitter Analysis\\033[0m' && ./latency_compare"]
