FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libncurses5-dev \
    libncursesw5-dev \
    trace-cmd \
    linux-tools-generic \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . /app/

RUN make clean && make

# Note: Container needs --privileged and host tracefs mounted
# docker run --privileged -v /sys/kernel/tracing:/sys/kernel/tracing ...

CMD ["/bin/bash"]
