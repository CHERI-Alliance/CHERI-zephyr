# CHERI-Zephyr-v0.3.0
# Copyright (c) 2025 University of Birmingham, Added to support CHERI spec
#
# SPDX-License-Identifier: Apache-2.0
#

#################################################################################
# Stage 1: Build tool image
#################################################################################

FROM ubuntu:24.04 AS cheri-toolbuilder
#FROM ghcr.io/zephyrproject-rtos/ci-repo-cache:v0.27.4.20241026 AS cheri-toolbuilder

# Define build arguments (defaults to "true") - needs to come after FROM or does not get set
ARG FEATURE_CHERIBUILD="true"
ARG FEATURE_CODASIP="true"
ARG FEATURE_CODASIP_PATCHED="true"
ARG TOOL_DIR="/home/user/"

# -----------------------------------
# Set non-interactive frontend for apt-get
# to skip any user confirmations and install
# dependencies
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && \
    apt upgrade -y && \
    apt install -y \
    # Build tools
    build-essential make gcc g++ gcc-multilib g++-multilib clang \
    autoconf automake libtool pkg-config ninja-build bison flex \
    cmake \
    # Python
    python3-dev python3-pip python3-setuptools python3-tk python3-wheel python3-venv python3-pyelftools \
    # Libraries / dev headers
    libglib2.0-dev libpixman-1-dev libsdl2-dev libarchive-dev libarchive-tools \
    libbz2-dev libattr1-dev libcap-ng-dev libexpat1-dev libgmp-dev libmpfr-dev libmpc-dev \
    libncurses-dev libdebuginfod-dev libmagic1 \
    # Utilities
    git dfu-util device-tree-compiler file xz-utils bzip2 time vim nano tree sudo \
    # Networking / other tools
    samba texinfo \
    #fpm .deb builder
    ruby ruby-dev rubygems

# fpm .deb builder
RUN gem install --no-document fpm

RUN ln -fs /usr/share/zoneinfo/Europe/London /etc/localtime && echo "Europe/London" > /etc/timezone

# -----------------------------------
# Create a non-root user - cheribuild causes fatal error if run as root
#RUN useradd -m -s /bin/bash user && \
#    echo "user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN sudo usermod -l user ubuntu && \
    sudo usermod -d /home/user -m user && \
    sudo groupmod -n user ubuntu && \
    echo "user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
#-----------------------------------

# Build for non-root
# Everything now relative to user directory
USER user

# Validate that at least one feature is enabled; otherwise, exit with an error
RUN if [ "$FEATURE_CODASIP" = "false" ] && [ "$FEATURE_CHERIBUILD" = "false" ] && [ "$FEATURE_CODASIP_PATCHED" = "false" ]; then \
        echo "ERROR: At least one feature must be enabled! Exiting..."; \
        exit 1; \
    fi || exit 1

#################### Install FEATURE_CHERIBUILD (only if enabled)
RUN mkdir -p $TOOL_DIR/cheribuild && \
    chown -R user:user $TOOL_DIR/cheribuild

WORKDIR $TOOL_DIR/cheribuild
RUN if [ "$FEATURE_CHERIBUILD" = "true" ]; then \
        git clone --no-checkout https://github.com/CTSRD-CHERI/cheribuild.git $TOOL_DIR/cheribuild && \
        git checkout df913531; \
    fi

RUN if [ "$FEATURE_CHERIBUILD" = "true" ]; then \
        ./cheribuild.py freestanding-cheri-sdk -d -v; \
    fi

# Install aditional cheribuild features / options
# baremetal
RUN if [ "$FEATURE_CHERIBUILD" = "true" ]; then \
        ./cheribuild.py -v compiler-rt-builtins-baremetal-riscv32 -d && \
        ./cheribuild.py -v compiler-rt-builtins-baremetal-riscv32-purecap -d && \
        ./cheribuild.py -v compiler-rt-builtins-baremetal-riscv64 -d && \
        ./cheribuild.py -v compiler-rt-builtins-baremetal-riscv64-purecap -d; \
    fi

# baremetal newlib
RUN if [ "$FEATURE_CHERIBUILD" = "true" ]; then \
        ./cheribuild.py -v compiler-rt-builtins-baremetal-newlib-riscv32 -d && \
        ./cheribuild.py -v compiler-rt-builtins-baremetal-newlib-riscv32-purecap -d && \
        ./cheribuild.py -v compiler-rt-builtins-baremetal-newlib-riscv64 -d && \
        ./cheribuild.py -v compiler-rt-builtins-baremetal-newlib-riscv64-purecap -d; \
    fi

# picolib
# NOTE: compiler-rt-builtins-picolibc-riscv32-purecap doesn't exist!
RUN if [ "$FEATURE_CHERIBUILD" = "true" ]; then \
        ./cheribuild.py -v compiler-rt-builtins-picolibc-riscv32 -d && \
        ./cheribuild.py -v compiler-rt-builtins-picolibc-riscv64 -d && \
        ./cheribuild.py -v compiler-rt-builtins-picolibc-riscv64-purecap -d; \
    fi

#################### Install FEATURE_CODASIP (only if enabled)

###### CODASIP-QEMU
RUN mkdir -p $TOOL_DIR/qemu-codasip
WORKDIR $TOOL_DIR/qemu-codasip

RUN if [ "${FEATURE_CODASIP}" = "true" ]; then \
        git clone --no-checkout --recurse-submodules --branch codasip-cheri-riscv_v3 https://github.com/CHERI-Alliance/qemu.git $TOOL_DIR/qemu-codasip && \
        git checkout 2a2e882b; \
    fi

# Copy patches for 32-bit codasip
COPY ./support/codasip-llvm-32bit-perms-patch.diff /tmp
COPY ./support/codasip-qemu-32bit-boot-code_patch.diff /tmp

RUN if [ "${FEATURE_CODASIP}" = "true" ] && [ "${FEATURE_CODASIP_PATCHED}" = "true" ]; then \
        git apply /tmp/codasip-qemu-32bit-boot-code_patch.diff; \
    fi

RUN if [ "${FEATURE_CODASIP}" = "true" ]; then \
        mkdir -p build && \
        cd build && \
        ../configure --target-list="riscv32cheri-softmmu riscv64cheri-softmmu" \
            --disable-gtk --audio-drv-list="" --disable-brlapi --disable-libiscsi \
            --disable-libnfs --disable-rbd --disable-sdl --disable-snappy \
            --disable-vnc --disable-vnc-jpeg --disable-vnc-sasl --disable-l2tpv3 \
            --disable-oss --disable-alsa --disable-tpm --disable-werror --meson=git && \
        ninja -j$(nproc); \
    fi

###### CODASIP-LLVM
WORKDIR $TOOL_DIR/llvm-cheri-codasip
RUN if [ "${FEATURE_CODASIP}" = "true" ]; then \
        git clone --no-checkout --recurse-submodules --branch codasip-cheri-riscv https://github.com/CHERI-Alliance/llvm-project.git \
            $TOOL_DIR/llvm-cheri-codasip && \
        git checkout 1ca584e7; \
    fi

RUN if [ "${FEATURE_CODASIP}" = "true" ] && [ "${FEATURE_CODASIP_PATCHED}" = "true" ]; then \
      git apply /tmp/codasip-llvm-32bit-perms-patch.diff; \
    fi

RUN if [ "${FEATURE_CODASIP}" = "true" ]; then \
        mkdir -p build && \
        cd build && \
        cmake -DCMAKE_INSTALL_PREFIX=/llvm-cheri-codasip/build \
            -DLLVM_TARGETS_TO_BUILD=RISCV \
            -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-elf" \
            -DLLVM_ENABLE_PROJECTS="clang;lld" \
            -DCMAKE_BUILD_TYPE=Release \
            -GNinja ../llvm/ \
            -DLLVM_PARALLEL_LINK_JOBS=2 \
            -DLLVM_USE_SPLIT_DWARF=ON \
            -DBUILD_SHARED_LIBS=ON \
            -Wno-dev && \
        ninja -j$(nproc); \
    fi

# Generate codasip libclang_rt files for various mabi / march combos (defined in script)
COPY ./support/build_compiler_rt.sh /tmp
RUN if [ "${FEATURE_CODASIP}" = "true" ]; then \
        /bin/bash /tmp/build_compiler_rt.sh;\
    fi

###### CODASIP-GDB
WORKDIR $TOOL_DIR/gdb-codasip
RUN if [ "${FEATURE_CODASIP}" = "true" ]; then \
        git clone --no-checkout --recurse-submodules --branch codasip-cheri-riscv https://github.com/CHERI-Alliance/gdb.git $TOOL_DIR/gdb-codasip && \
        git checkout df929d4d; \
    fi

RUN if [ "${FEATURE_CODASIP}" = "true" ]; then \
        mkdir -p build && \
        ./configure --target=riscv64-unknown-elf --prefix=$TOOL_DIR/gdb-codasip/build && \
        make && \
        make install; \
    fi

###### CODASIP-QEMU-PATCHED
COPY ./support/qemu-patch-second-uart.diff /tmp
RUN mkdir -p $TOOL_DIR/qemu-codasip-patched

WORKDIR $TOOL_DIR/qemu-codasip-patched
RUN if [ "${FEATURE_CODASIP_PATCHED}" = "true" ]; then \
        git clone --no-checkout --recurse-submodules --branch codasip-cheri-riscv_v3 https://github.com/CHERI-Alliance/qemu.git $TOOL_DIR/qemu-codasip-patched && \
        git checkout 2a2e882b && \
        git apply /tmp/codasip-qemu-32bit-boot-code_patch.diff && \
        git apply /tmp/qemu-patch-second-uart.diff; \
    fi


RUN if [ "${FEATURE_CODASIP_PATCHED}" = "true" ]; then \
        mkdir -p build && \
        cd build && \
        ../configure --target-list="riscv32cheri-softmmu riscv64cheri-softmmu" \
            --disable-gtk --audio-drv-list="" --disable-brlapi --disable-libiscsi \
            --disable-libnfs --disable-rbd --disable-sdl --disable-snappy \
            --disable-vnc --disable-vnc-jpeg --disable-vnc-sasl --disable-l2tpv3 \
            --disable-oss --disable-alsa --disable-tpm --disable-werror --meson=git && \
        ninja -j$(nproc); \
    fi

#LLVM

# link the 4 libclang_rt.a files you want for codasip 32/64 and nocap/purecap
RUN mkdir -p $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv32
RUN cd $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv32 && \
    ln -s ../../../../llvm-builtins-codasip/rv32imac-ilp32/lib/baremetal lib

RUN mkdir -p $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv64
RUN cd $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv64 && \
    ln -s ../../../../llvm-builtins-codasip/rv64imac-lp64/lib/baremetal lib

RUN mkdir -p $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv32-purecap
RUN cd $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv32-purecap && \
    ln -s ../../../../llvm-builtins-codasip/rv32imazcheripurecap-il32pc64/lib/baremetal lib

RUN mkdir -p $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv64-purecap
RUN cd $TOOL_DIR/llvm-cheri-codasip/build/baremetal/baremetal-riscv64-purecap && \
    ln -s ../../../../llvm-builtins-codasip/rv64imazcheripurecap-l64pc128/lib/baremetal lib

# -----------------------------------
# Create .deb packages for the built tools
# Cheribuild
RUN if [ "$FEATURE_CHERIBUILD" = "true" ]; then \
        cd ~ && \
        fpm -s dir --deb-compression gz -t deb -n cheribuild -v 0.0.2 --prefix=/opt/cheri cheri/output ; \
    fi


RUN if [ "$FEATURE_CODASIP" = "true" ]; then \
        cd ~ && \
        fpm -s dir --deb-compression gz -t deb -n codasip-llvm -v 0.0.2 --prefix=/opt/cheri llvm-builtins-codasip llvm-cheri-codasip/build ; \
    fi

# GDB
RUN if [ "$FEATURE_CODASIP" = "true" ]; then \
        cd ~ && \
        fpm -s dir --deb-compression gz -t deb -n codasip-gdb -v 0.0.2 --prefix=/opt/cheri gdb-codasip/gdb ; \
    fi

# QEMU
RUN if [ "$FEATURE_CODASIP" = "true" ]; then \
        cd ~ && \
        fpm -s dir --deb-compression gz -t deb -n codasip-qemu -v 0.0.2 --prefix=/opt/cheri qemu-codasip/build ; \
    fi

# QEMU-patched
RUN if [ "$FEATURE_CODASIP_PATCHED" = "true" ]; then \
        cd ~ && \
        fpm -s dir --deb-compression gz -t deb -n codasip-qemu-patched -v 0.0.2 --prefix=/opt/cheri qemu-codasip-patched/build ; \
    fi

RUN cd ~ && \
    mkdir -p output && \
    mv *.deb output/
