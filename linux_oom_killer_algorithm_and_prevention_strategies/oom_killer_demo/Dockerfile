FROM ubuntu:22.04
LABEL maintainer="Systems Programming Deep Dive"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
        build-essential \
        libncurses5-dev \
        libncursesw5-dev \
        procps \
        psmisc \
        util-linux \
        librsvg2-bin \
        imagemagick \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN make

CMD ["/bin/bash"]
