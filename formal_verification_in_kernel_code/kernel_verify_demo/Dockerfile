FROM ubuntu:22.04
LABEL description="Kernel Verification Demo — Systems Programming Deep Dive"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    clang          \
    llvm           \
    libncurses-dev \
    make           \
    libc-dev       \
    linux-tools-generic \
    strace         \
    valgrind       \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /demo

COPY kasan_sim.c lockdep_sim.c bpf_verify_sim.c kcsan_sim.c monitor.c Makefile ./

RUN make all 2>&1 | tail -20

CMD ["/bin/bash", "-c", \
     "echo '=== KASAN Simulation ===' && ./kasan_sim; \
      echo ''; echo '=== lockdep Simulation ===' && ./lockdep_sim; \
      echo ''; echo '=== BPF Verifier Simulation ===' && ./bpf_verify_sim; \
      echo ''; echo '=== KCSAN/Race Simulation ===' && ./kcsan_sim"]
