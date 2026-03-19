# CHERI-Zephyr-v0.3.0
# Copyright (c) 2025 University of Birmingham, Added to support CHERI spec
#
# SPDX-License-Identifier: Apache-2.0
#

#################################################################################
# Stage 2: Create runtime image
# Requires Stage 1: cheri-llvm and
# Requires external: zephyrproject from https://github.com/cheri-zephyr-project/CHERI-zephyr/ as ./zephyr
#################################################################################

FROM ubuntu:24.04 AS cheri-zephyr-west

ARG HOME_DIR="/home/user"
ARG TOOL_DIR="/opt/cheri"

# -----------------------------------
# Set non-interactive frontend for apt-get
# to skip any user confirmations and install
# dependencies
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt upgrade -y && \
    apt install -y \
    # Build tools
    autoconf automake bison build-essential cmake flex gcc g++ gcc-multilib g++-multilib \
    clang libtool ninja-build gperf pkg-config \
    # Python
    python3-dev python3-pip python3-setuptools python3-tk python3-wheel python3-venv \
    # Utilities / tools
    dfu-util device-tree-compiler file git nano sudo tzdata xz-utils tree vim nano \
    # Libraries / headers
    libsdl2-dev libmagic1 libdebuginfod-dev \
    # Terminal stuff
    tmux netcat-openbsd

RUN ln -fs /usr/share/zoneinfo/Europe/London /etc/localtime && echo "Europe/London" > /etc/timezone

# -----------------------------------
# Create a non-root user - cheribuild causes fatal error if run as root
#RUN useradd -m -s /bin/bash user && \
#    echo "user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN sudo usermod -l user ubuntu && \
    sudo usermod -d /home/user -m user && \
    sudo groupmod -n user ubuntu && \
    echo "user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
# -----------------------------------

# -----------------------------------
# Ensure the correct permissions before 
# switching users (-p for multiple levels)
RUN mkdir -p $TOOL_DIR/cheri/output && \
    chown -R user:user $TOOL_DIR/cheri && \
    mkdir -p $TOOL_DIR/qemu-codasip/build && \
    chown -R user:user $TOOL_DIR/qemu-codasip && \
    mkdir -p $TOOL_DIR/qemu-codasip-patched/build && \
    chown -R user:user $TOOL_DIR/qemu-codasip-patched && \
    mkdir -p $TOOL_DIR/llvm-cheri-codasip/build && \
    chown -R user:user $TOOL_DIR/llvm-cheri-codasip && \
    mkdir -p $TOOL_DIR/gdb-codasip/build && \
    chown -R user:user $TOOL_DIR/gdb-codasip && \
    mkdir -p $TOOL_DIR/zephyrproject/zephyr && \
    chown -R user:user $TOOL_DIR/zephyrproject && \
    mkdir -p $TOOL_DIR/custom-debs
# -----------------------------------

# Import & intall DEBs
RUN mkdir $HOME_DIR/custom-debs
COPY custom-debs/*.deb $HOME_DIR/custom-debs/

RUN dpkg -i $HOME_DIR/custom-debs/cheribuild_0.0.2_amd64.deb $HOME_DIR/custom-debs/codasip-gdb_0.0.2_amd64.deb $HOME_DIR/custom-debs/codasip-llvm_0.0.2_amd64.deb $HOME_DIR/custom-debs/codasip-qemu_0.0.2_amd64.deb $HOME_DIR/custom-debs/codasip-qemu-patched_0.0.2_amd64.deb; 

# link the 4 libclang_rt.a files you want for codasip 32/64 and nocap/purecap
RUN mkdir -p $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv32
RUN ln -s $TOOL_DIR/llvm-builtins-codasip/rv32imac-ilp32/lib/baremetal \
          $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv32/lib

RUN mkdir -p $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv64
RUN ln -s $TOOL_DIR/llvm-builtins-codasip/rv64imac-lp64/lib/baremetal \
          $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv64/lib

RUN mkdir -p $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv32-purecap
RUN ln -s $TOOL_DIR/llvm-builtins-codasip/rv32imazcheripurecap-il32pc64/lib/baremetal \
          $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv32-purecap/lib

RUN mkdir -p $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv64-purecap
RUN ln -s $TOOL_DIR/llvm-builtins-codasip/rv64imazcheripurecap-l64pc128/lib/baremetal \
          $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv64-purecap/lib

# -----------------------------------

# Build for non-root
# Everything now relative to user directory
USER user
# -----------------------------------

COPY docker-release/support/*.sh $HOME_DIR/zephyrproject/
COPY docker-release/support/*.py $HOME_DIR/zephyrproject/
COPY zephyr/scripts/requirements*.txt $HOME_DIR/zephyrproject/

USER root
RUN chown -R user:user  $HOME_DIR/zephyrproject
RUN chmod u+x $HOME_DIR/zephyrproject/*.sh \
   && chmod u+x $HOME_DIR/zephyrproject/*.py

# -----------------------------------
# Zephyr west install in python venv
USER user

WORKDIR $HOME_DIR/zephyrproject

RUN python3 -m venv .venv && \
    . .venv/bin/activate && \
    pip install pyelftools && \
    pip install west && \
    pip install -r $HOME_DIR/zephyrproject/requirements.txt 
# -----------------------------------

CMD ["bash"]
#################################################################################
