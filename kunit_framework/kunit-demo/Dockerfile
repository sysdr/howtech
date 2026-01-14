FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    bc \
    kmod \
    cpio \
    flex \
    bison \
    libelf-dev \
    libssl-dev \
    libncurses-dev \
    wget \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Download and extract Linux kernel (use recent stable version)
WORKDIR /kernel
RUN wget -q https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.6.tar.xz && \
    tar xf linux-6.6.tar.xz && \
    rm linux-6.6.tar.xz

WORKDIR /kernel/linux-6.6

# Enable KUnit in kernel config
RUN make ARCH=um defconfig && \
    echo "CONFIG_KUNIT=y" >> .config && \
    echo "CONFIG_KUNIT_TEST=m" >> .config && \
    echo "CONFIG_KUNIT_EXAMPLE_TEST=n" >> .config

# Copy our driver code
COPY driver.h driver_ops.h driver.c driver_real.c driver_mock.c driver_test.c \
     Kconfig Makefile /kernel/kunit-demo/

# Integrate with kernel build system
RUN mkdir -p drivers/kunit_demo && \
    cp /kernel/kunit-demo/* drivers/kunit_demo/ && \
    echo 'source "drivers/kunit_demo/Kconfig"' >> drivers/Kconfig && \
    echo 'obj-y += kunit_demo/' >> drivers/Makefile && \
    echo "CONFIG_KUNIT_DEMO_DRIVER=m" >> .config && \
    echo "CONFIG_KUNIT_DEMO_TESTS=m" >> .config

# Build UML kernel
RUN make ARCH=um olddefconfig && \
    make ARCH=um -j$(nproc)

WORKDIR /workspace

CMD ["/bin/bash"]
