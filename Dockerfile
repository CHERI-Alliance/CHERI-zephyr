FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV ZEPHYR_SDK_INSTALL_DIR=/zephyr-sdk-1.0.1

RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    cmake \
    ninja-build \
    gperf \
    ccache \
    dfu-util \
    device-tree-compiler \
    wget \
    python3-dev \
    python3-venv \
    python3-tk \
    xz-utils \
    file \
    make \
    gcc \
    gcc-multilib \
    g++-multilib \
    libsdl2-dev \
    libmagic1 \
    patch \
    gnupg2 \
    lsb-release \
    && rm -rf /var/lib/apt/lists/*

RUN wget -q -O /tmp/llvm-sig.gpg https://apt.llvm.org/llvm-snapshot.gpg.key \
    && gpg --dearmor -o /etc/apt/keyrings/llvm.gpg /tmp/llvm-sig.gpg \
    && rm /tmp/llvm-sig.gpg \
    && echo "deb [signed-by=/etc/apt/keyrings/llvm.gpg] http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-22 main" \
       > /etc/apt/sources.list.d/llvm.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
       clang-22 \
       libclang-22-dev \
       libclang-cpp22 \
       libclang-cpp22-dev \
       llvm-22 \
       llvm-22-dev \
       libllvm22 \
    && rm -rf /var/lib/apt/lists/*

RUN wget -q https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v1.0.1/zephyr-sdk-1.0.1_linux-x86_64_gnu.tar.xz \
    && tar xaf zephyr-sdk-1.0.1_linux-x86_64_gnu.tar.xz \
    && rm zephyr-sdk-1.0.1_linux-x86_64_gnu.tar.xz

RUN cd zephyr-sdk-1.0.1 \
    && ./setup.sh -h -c

# Compatibility symlink for TF-M / NCS builds expecting old SDK layout
RUN ln -s gnu/arm-zephyr-eabi /zephyr-sdk-1.0.1/arm-zephyr-eabi

WORKDIR /work