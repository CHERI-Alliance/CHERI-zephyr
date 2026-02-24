#!/bin/bash

# A few constants
LLVM_BIN_PATH=/home/$(whoami)/llvm-cheri-codasip/build/bin
INSTALL_ROOT=/home/$(whoami)/llvm-builtins-codasip
CAPS_EXTRA_INIT_FLAGS="-Werror=cheri-capability-misuse -Werror=cheri-bitwise-operations -Werror=cheri-prototypes -Werror=pass-failed"

# Define the various MARCH/MABI combos we want to build.  Format:
#
#        ARCH    MARCH                        MABI          EXTRA_INIT_FLAGS
#
# Jesse's initial values
conf1=( '32'     'rv32imac'                   'ilp32'       "")                          # 32 bit, no caps
conf2=( '32'     'rv32imacxcheri'             'il32pc64'    "$CAPS_EXTRA_INIT_FLAGS")    # 32 bit, caps
conf3=( '64'     'rv64imac'                   'lp64'        "")                          # 64 bit, no caps
conf4=( '64'     'rv64imacxcheri'             'l64pc128'    "$CAPS_EXTRA_INIT_FLAGS")    # 64 bit, caps

# Jen's old suggestions
conf5=( '32'     'rv32gc'                     'ilp32d'      "")                          # 32 bit, no caps
conf6=( '32'     'rv32imafdzcheripurecap'     'il32pc64'    "$CAPS_EXTRA_INIT_FLAGS")    # 32 bit, caps
conf7=( '64'     'rv64gc'                     'lp64d'       "")                          # 64 bit, no caps
conf8=( '64'     'rv64imafdzcheripurecap'     'l64pc128'    "$CAPS_EXTRA_INIT_FLAGS")    # 64 bit, caps

# new options
conf9=( '32'     'rv32imazcheripurecap'       'il32pc64'    "$CAPS_EXTRA_INIT_FLAGS")    # 32 bit, caps
conf10=('64'     'rv64imazcheripurecap'       'l64pc128'    "$CAPS_EXTRA_INIT_FLAGS")    # 64 bit, caps
conf11=('32'     'rv32imafdzcheripurecap'     'il32pc64d'   "$CAPS_EXTRA_INIT_FLAGS")    # 32 bit, caps
conf12=('64'     'rv64imafdzcheripurecap'     'l64pc128d'   "$CAPS_EXTRA_INIT_FLAGS")    # 64 bit, caps


# NOTE: Don't forget to add the configs here if you want them to be included!
configs=(conf1 conf2 conf3 conf4 conf5 conf6 conf7 conf8 conf9 conf10 conf11 conf12)

declare -n conf

for conf in "${configs[@]}"; do
    ARCH=${conf[0]}
    TARGET_TRIPLE="riscv$ARCH-unknown-elf" #build TARGET_TRIPLE from ARCH
    MARCH=${conf[1]}
    MABI=${conf[2]}
    INIT_FLAGS=${conf[3]}

    # Unique build directory
    INSTALL_DIR=${INSTALL_ROOT}/${MARCH}-${MABI}
    BUILD_DIR=/home/$(whoami)/llvm-cheri-codasip/build_comiler_rt_${MARCH}_${MABI}

    echo "##### SUMMARY #####"
    echo "TARGET TRIPLE: $TARGET_TRIPLE"
    echo "MARCH: $MARCH"
    echo "MABI: $MABI"
    echo "INIT_FLAGS: $INIT_FLAGS"
    echo "INSTALL_DIR: $INSTALL_DIR"
    echo "BUILD_DIR: $BUILD_DIR"
    echo "###################"

    mkdir -p  ${BUILD_DIR}
    cd ${BUILD_DIR}
    cmake ../compiler-rt \
        -GNinja \
        -S /home/$(whoami)/llvm-cheri-codasip/compiler-rt/lib/builtins \
        -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
        -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
        -DCMAKE_C_COMPILER=${LLVM_BIN_PATH}/clang \
        -DCMAKE_AR=${LLVM_BIN_PATH}/llvm-ar \
        -DCMAKE_NM=${LLVM_BIN_PATH}/llvm-nm \
        -DCMAKE_LINKER=${LLVM_BIN_PATH}/ld.lld \
        -DCMAKE_C_COMPILER=${LLVM_BIN_PATH}/clang \
        -DCMAKE_CXX_COMPILER=${LLVM_BIN_PATH}/clang++ \
        -DCMAKE_ASM_COMPILER=${LLVM_BIN_PATH}/clang \
        -DCMAKE_RANLIB=${LLVM_BIN_PATH}/llvm-ranlib \
        -DCMAKE_BUILD_RPATH_USE_ORIGIN=TRUE \
        -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE \
        -DCMAKE_INSTALL_RPATH=$ORIGIN/../lib \
        -DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE \
        -DCOMPILER_RT_BAREMETAL_BUILD=ON \
        -DCOMPILER_RT_DEBUG=TRUE \
        -DCOMPILER_RT_EXCLUDE_ATOMIC_BUILTIN=FALSE \
        -DLLVM_CONFIG_PATH=${LLVM_BIN_PATH}/llvm-config \
        -DCOMPILER_RT_OS_DIR="baremetal" \
        -DCMAKE_C_COMPILER_TARGET="${TARGET_TRIPLE}" \
        -DCMAKE_ASM_COMPILER_TARGET="${TARGET_TRIPLE}" \
        -DCMAKE_C_FLAGS="--target=${TARGET_TRIPLE} -march=${MARCH} -mabi=${MABI}" \
        -DCMAKE_ASM_FLAGS="--target=${TARGET_TRIPLE} -march=${MARCH} -mabi=${MABI}" \
        "-DCMAKE_C_FLAGS_INIT=            -target ${TARGET_TRIPLE} -B${LLVM_BIN_PATH} -march=${MARCH} -mabi=${MABI} -mno-relax -mcmodel=medany -ggdb -gz -Wno-error=unused-command-line-argument -ffreestanding -Werror=implicit-function-declaration -Werror=format -Werror=incompatible-pointer-types -Werror=undefined-internal ${EXTRA_INIT_FLAGS}" \
        "-DCMAKE_ASM_FLAGS_INIT=          -target ${TARGET_TRIPLE} -B${LLVM_BIN_PATH} -march=${MARCH} -mabi=${MABI} -mno-relax -mcmodel=medany -ggdb -gz -Wno-error=unused-command-line-argument -ffreestanding -Werror=implicit-function-declaration -Werror=format -Werror=incompatible-pointer-types -Werror=undefined-internal ${EXTRA_INIT_FLAGS}" \
        "-DCMAKE_EXE_LINKER_FLAGS_INIT=   -target ${TARGET_TRIPLE} -B${LLVM_BIN_PATH} -march=${MARCH} -mabi=${MABI} -mno-relax -mcmodel=medany -fuse-ld=lld --ld-path=${LLVM_BUILD_PATH}/ld.lld -Wl,--gdb-index -Wl,--compress-debug-sections=zlib" \
        "-DCMAKE_SHARED_LINKER_FLAGS_INIT=-target ${TARGET_TRIPLE} -B${LLVM_BIN_PATH} -march=${MARCH} -mabi=${MABI} -mno-relax -mcmodel=medany -fuse-ld=lld --ld-path=${LLVM_BUILD_PATH}/ld.lld -Wl,--gdb-index -Wl,--compress-debug-sections=zlib" \
        "-DCMAKE_MODULE_LINKER_FLAGS_INIT=-target ${TARGET_TRIPLE} -B${LLVM_BIN_PATH} -march=${MARCH} -mabi=${MABI} -mno-relax -mcmodel=medany -fuse-ld=lld --ld-path=${LLVM_BUILD_PATH}/ld.lld -Wl,--gdb-index -Wl,--compress-debug-sections=zlib"

    cd ${BUILD_DIR} && ninja -j$(nproc) && ninja install
done
exit
