FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /demo

# Copy source files
COPY src/ src/
COPY Makefile .

# Build programs
RUN make all

# Run latency test by default
CMD ["./build/latency_test"]
