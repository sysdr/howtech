FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    gcc \
    make \
    strace \
    valgrind \
    linux-tools-generic \
    procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Increase message queue limits
RUN echo "fs.mqueue.msg_max = 2048" >> /etc/sysctl.conf && \
    echo "fs.mqueue.msgsize_max = 65536" >> /etc/sysctl.conf

# Increase shared memory size
RUN echo "tmpfs /dev/shm tmpfs defaults,size=512m 0 0" >> /etc/fstab

COPY . /app

RUN make clean && make

CMD ["/bin/bash"]
