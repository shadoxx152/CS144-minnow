FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    git \
    cmake \
    gdb \
    build-essential \
    clang \
    clang-tidy \
    clang-format \
    gcc-doc \
    pkg-config \
    glibc-doc \
    tcpdump \
    tshark \
    g++-13 \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

WORKDIR /cs144

CMD ["/bin/bash"]
