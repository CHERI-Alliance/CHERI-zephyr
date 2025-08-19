*This work was funded by the Digital Security by Design (DSbD) Programme delivered by UKRI to support the DSbD ecosystem, and the University of Birmingham EPSRC IAA grant.*

*The zephyr project is released under an Apache 2.0 LICENSE.*

*This README.md file includes CHERI-specific information. Refer to the main Zephyr README.rst for further information.*

# CHERI - Zephyr - release CHERI-RISCV64-v2.0

This release is a rebase version of CHERI-Zephyr (CHERI-RISCV64-v1.0) with a more recent upstream version of Zephyr (Commit 90a48e8, May 20, 2025 - based on Zephyr 4.1).

CHERI-Zephyr along with the CHERI-based tool chains can be installed manually following the section on `Detailed Manual Setup` or can be built and run using Docker following the section on `Docker Setup`.

Limitations: This version currently provides minimimal CHERI support to run the applications listed. It does not yet provide CHERI support to every Zephyr feature.

## CHERI Support

CHERI support is provided for system emulation of CHERI RISC-V 64-bit machines. The following have been supported:

**Cambridge CHERI-RISCV64**

* cheribuild (QEMU, LLVM, GDB): https://github.com/CTSRD-CHERI/cheribuild.git (Commit: df913531)

* specs: CHERI-ISA: https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-951.pdf


**Codasip CHERI-RISCV64 ZCHERIPURECAP v0.9.5 spec**

* QEMU: https://github.com/CHERI-Alliance/qemu/tree/codasip-cheri-riscv_v3
(branch: codasip-cheri-riscv-v3, Commit: 2a2e882b)

* LLVM: https://github.com/CHERI-Alliance/llvm-project/tree/codasip-cheri-riscv
(branch: codasip-cheri-riscv, Commit: 1ca584e7)

* GDB: https://github.com/CHERI-Alliance/gdb/tree/codasip-cheri-riscv
(branch: codasip-cheri-riscv, Commit: df929d4d)

* v0.9.5 spec: https://github.com/riscv/riscv-cheri/releases/tag/v0.9.5 

## CHERI Board Support in Zephyr

The following CHERI boards are supported:

* build normal RISCV for QEMU-CHERI-RISCV64 (Cambridge/Codasip):
    * qemu_riscv64cheri
    * qemu_riscv64cheri_smp
* build RISCV with CHERI Capabilities (purecap only) for QEMU-CHERI-RISCV64 (Cambridge):
    * qemu_riscv64cheri_purecap
    * qemu_riscv64cheri_smp_purecap
* build RISCV with CHERI Capabilities (zcheripurecap only) for QEMU-CHERI-RISCV64 (Codasip):
    * qemu_riscv64cheri_zcheripurecap
    * qemu_riscv64cheri_smp_zcheripurecap

## Invoking the CHERI toolchain for building Zephyr

The toolchain can be invoked by either including the following environment variables or modifying the `home/.zephyrrc` file with:

* Cambridge QEMU-CHERI-RISCV64:
```
export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
export LLVM_CHERI_TOOLCHAIN_PATH= <path>/cheri/output/sdk
export QEMU_BIN_PATH= <path>/cheri/output/sdk/bin
```
* Codasip QEMU-CHERI-RISCV64 (0.9.5 spec):
```
export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
export LLVM_CHERI_TOOLCHAIN_PATH=<path>/llvm-cheri-codasip/build
export QEMU_BIN_PATH=<path>/qemu-codasip/build
```

## Running CHERI-Zephyr sample applications

Assumming Zephyr has been installed to `zephyrproject/zephyr` (see section on `Detailed Manual Setup`) then open a new terminal and set up the environment with:

```
cd zephyrproject/zephyr/
source ~/zephyrproject/.venv/bin/activate
source zephyr-env.sh
```

* build with west for normal RISCV for QEMU-CHERI-RISCV64 (Cambridge/Codasip):
```
west build -p always -b qemu_riscv64cheri samples/hello_world
west build -t run
```
output:
```
-- west build: running target run
[0/1] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: riscv64cheri
*** Booting Zephyr OS build zephyr-v3.5.0-1112-ge73bafc6c501 ***
Hello World! qemu_riscv64cheri
```

* build with west for RISCV with CHERI Capabilities (purecap only) for QEMU-CHERI-RISCV64 (Cambridge):
```
west build -p always -b qemu_riscv64cheri_purecap samples/hello_world
west build -t run
```
output:
```
-- west build: running target run
[0/1] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: (README.rst) riscv64cheri
*** Booting Zephyr OS build zephyr-v3.5.0-1112-ge73bafc6c501 ***
Hello World! qemu_riscv64cheri_purecap
```

* build with west for RISCV with CHERI Capabilities (zcheripurecap only) for QEMU-CHERI-RISCV64 (Codasip):
```
west build -p always -b qemu_riscv64cheri_zcheripurecap samples/hello_world
west build -t run
```
Output:
```
-- west build: running target run
[0/1] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: riscv64cheri
*** Booting Zephyr OS build zephyr-v3.5.0-1112-ge73bafc6c501 ***
Hello World! qemu_riscv64cheri_zcheripurecap
```

The following sample apps work with west with
the current CHERI modifications: `hello_world, synchronization,
philosophers, basic/sys_heap`.

Further information regarding set up is given below. Refer to the Zephyr README documentation for more Zephyr specific information. 

## Docker Setup

This section describes set up for a docker container for testing with the CHERI toolchain or the Zephyr SDK. (See section on `Detailed Manual Setup` for a manual install.)

First clone the release branch to ~/zephyrproject/zephyr
```
git clone --recurse-submodules --branch <branch name> <git repo> ~/zephyrproject/zephyr
```
Ensure you have docker installed, for Ubuntu:

```
sudo apt install -y docker.io
```
Then follow the [docker instructions](runningdocker.md) to run the dockerfile for the current release.


## Detailed Manual Setup

This section describes a manual setup.

*Note - This set up has only been tested with Ubuntu 22.04.*

* Cambridge QEMU-CHERI-RISCV64:
  * Step1: Clone cheribuild repo
  * Step2: Install dependencies
  * Step3: Install qemu, llvm, gdb
* Codasip QEMU-CHERI-RISCV64 (0.9.5 spec):
  * Step1: Clone codasip repos
  * Step2: Install dependencies
  * Step3: Install qemu-codasip
  * Step4: Install llvm-cheri-codasip (include -DLLVM_DEFAULT_TARGET_TRIPLE)
  * step5: Install gdb-codasip (optional for debugging)
* zephyr (this repo):
  * Step1: Clone zephyr (this branch/repo)
  * Step2: Install dependencies
  * Step3: set up environment and install west
  * Step4: Build Manual cloned fork of CHERI-Zephyr
  * Step5: Set up tool chain
  * Step6: Build and run sample hello_world

### Cambridge QEMU-CHERI-RISCV64:

**Step1: Clone cheribuild repo**

clone cheribuild with commit id df913531 to `~/cheribuild`. See https://github.com/CTSRD-CHERI/cheribuild for more information.

```
git clone --no-checkout https://github.com/CTSRD-CHERI/cheribuild.git ~/cheribuild
cd ~/cheribuild
git checkout df913531
```
**Step2: Install dependencies**

```
sudo apt update
sudo apt upgrade -y
sudo apt install autoconf automake libtool pkg-config clang bison cmake ninja-build samba flex texinfo time libglib2.0-dev libpixman-1-dev libarchive-dev libarchive-tools libbz2-dev libattr1-dev libcap-ng-dev
sudo apt install libexpat1-dev
sudo apt install libgmp-dev
```
**Step3: Install qemu, llvm, gdb**

Build CHERI for freestanding baremetal binaries:
```
cd ~/cheribuild
./cheribuild.py freestanding-cheri-sdk -d -v
```
output when done:
```
Built for target 'freestanding-cheri-sdk'
```


### Codasip QEMU-CHERI-RISCV64 (0.9.5 spec):
**Step1: Clone codasip toolchain repos.** 

Clone qemu to `~/qemu-codasip`, llvm-project to `~/llvm-cheri-codasip`, and gdb to `~/gdb-codasip`. See https://github.com/CHERI-Alliance for more information.


```
git clone --recurse-submodules --branch codasip-cheri-riscv_v3 https://github.com/CHERI-Alliance/qemu.git ~/qemu-codasip
git clone --recurse-submodules --branch codasip-cheri-riscv https://github.com/CHERI-Alliance/llvm-project.git ~/llvm-cheri-codasip
git clone --recurse-submodules --branch codasip-cheri-riscv https://github.com/CHERI-Alliance/gdb.git ~/gdb-codasip
```

Alternatively you can use `git checkout` for a specific commit. The following commit refs have been tested against:

```
git clone --no-checkout --recurse-submodules --branch codasip-cheri-riscv_v3 https://github.com/CHERI-Alliance/qemu.git ~/qemu-codasip
cd /qemu-codasip
git checkout 2a2e882b


git clone --no-checkout --recurse-submodules --branch codasip-cheri-riscv https://github.com/CHERI-Alliance/llvm-project.git ~/llvm-cheri-codasip
cd /llvm-cheri-codasip
git checkout 1ca584e7

git clone --no-checkout --recurse-submodules --branch codasip-cheri-riscv https://github.com/CHERI-Alliance/gdb.git ~/gdb-codasip
cd /gdb-codasip
git checkout df929d4d
```

**Step2: Install dependencies**
```
sudo apt update
sudo apt upgrade -y
sudo apt install autoconf automake libtool pkg-config clang bison cmake ninja-build samba flex texinfo time libglib2.0-dev libpixman-1-dev libarchive-dev libarchive-tools libbz2-dev libattr1-dev libcap-ng-dev libexpat1-dev libgmp-dev python3-pip libgmp-dev libmpfr-dev libmpc-dev
pip install pyelftools
```

**Step3: Install qemu-codasip**
```
cd /qemu-codasip
 mkdir build
 cd build
 ../configure --target-list="riscv32cheri-softmmu riscv64cheri-softmmu" \
   --disable-gtk --audio-drv-list="" --disable-brlapi --disable-libiscsi \
   --disable-libnfs --disable-rbd --disable-sdl --disable-snappy \
   --disable-vnc --disable-vnc-jpeg --disable-vnc-sasl --disable-l2tpv3 \
   --disable-oss --disable-alsa --disable-tpm --disable-werror --meson=git
 ninja
```
**Step4: Install llvm-cheri-codasip (include -DLLVM_DEFAULT_TARGET_TRIPLE)**

Note! - include -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-elf" otherwise causes `unknown triple` build error for Zephyr. 

```
cd /llvm-cheri
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/home/<path>/llvm-cheri-codasip/build -DLLVM_TARGETS_TO_BUILD=RISCV -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-elf" -DLLVM_ENABLE_PROJECTS="clang;lld" -DCMAKE_BUILD_TYPE=Release -GNinja ../llvm/ -DLLVM_PARALLEL_LINK_JOBS=1 -DLLVM_USE_SPLIT_DWARF=ON -DBUILD_SHARED_LIBS=ON -Wno-dev
ninja -j1
```
**step5: Install gdb-codasip (optional for debugging)**

```
cd /gdb-codasip
mkdir build
./configure --target=riscv64-unknown-elf --prefix=/<path>/gdb-codasip/build
make
make install
```

### zephyr (this release branch):
**Step1: Clone zephyr (this release branch/repo)**

Clone to ~/zephyrproject/zephyr
```
git clone --recurse-submodules --branch <release branch name> <this git repo> ~/zephyrproject/zephyr
```
**Step2: Install dependencies**

```
wget https://apt.kitware.com/kitware-archive.sh
sudo bash kitware-archive.sh
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
  make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1
sudo apt install python3-venv  
```

**Step3: Set up environment and install west**

Create a new python virtual environment in zephyrproject directory:

```
cd ~/zephyrproject
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate
```
Once activated your shell will be prefixed with `(.venv)`. 

Install west in the virtual environment in the zephyrproject directory.

```
pip install west
```

## Step4: Build Manual clone of CHERI-Zephyr

Inside the virtual environment, initialize the west workspace for a manual cloned fork of the Zephyr repository. (Using the -l option in the  command line)

```
west init -l ~/zephyrproject/zephyr
```

Fetch additional zephyr modules, export a Zephyr CMake package for loading boilerplate code for building Zephyr applications, and install additional Python dependencies.
```
west update
west zephyr-export
pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt
```

## Step5: Set up tool chain

create `.zephyrrc` file in home directory
```
#-------------------------------------
# CUSTOM CMAKE TOOLCHAINS
#-------------------------------------
##llvm-cheri cambridge
#export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
#export LLVM_CHERI_TOOLCHAIN_PATH=/home/<path>/cheri/output/sdk
#export QEMU_BIN_PATH=/home/<path>/cheri/output/sdk/bin

##llvm-cheri codasip
export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
export LLVM_CHERI_TOOLCHAIN_PATH=/home/<path>/llvm-cheri-codasip/build
export QEMU_BIN_PATH=/home/<path>/qemu-codasip/build
```

## Step6: Build and run sample hello_world

Open a new terminal.

```
cd zephyrproject/zephyr/
source ~/zephyrproject/.venv/bin/activate
source zephyr-env.sh
west build -p always -b qemu_riscv64cheri samples/hello_world
west build -t run
```
output:
```
-- west build: running target run
[0/1] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: riscv64cheri
*** Booting Zephyr OS build zephyr-v3.5.0-1112-ge73bafc6c501 ***
Hello World! qemu_riscv64cheri
```
