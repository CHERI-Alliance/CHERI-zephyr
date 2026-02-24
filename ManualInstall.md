# Manual Installation Instructions

This section describes a manual setup, for example if you want to install onto a VM.

Follow the [Docker instructions](README.md) to build the current release using Docker. 

*Note - This set up has only been tested with Ubuntu 22.04.*

* CHERI Toolchains and specs
* Patches (from this repo)
  * Step1: Get patches
* Cambridge QEMU-CHERI-RISCV:
  * Step1: Clone cheribuild repo
  * Step2: Install dependencies
  * Step3: Install qemu, llvm, gdb
  * Step4: Install builtins
* Codasip QEMU-CHERI-RISCV (0.9.5 spec):
  * Step1: Clone codasip repos
  * Step2: Install dependencies
  * Step3: Install patches
  * Step4: Install qemu-codasip
  * Step5: Install llvm-cheri-codasip
  * Step6: Install builtins
  * step7: Install gdb-codasip (optional for debugging)
* Zephyr (from this repo):
  * Step1: Clone zephyr (this branch/repo)
  * Step2: Install dependencies
  * Step3: set up environment and install west
  * Step4: Build Manual cloned fork of CHERI-Zephyr
  * Step5: Set up tool chain
  * Step6: Build and run sample hello_world

## CHERI Toolchains and specs

To support both the Codasip and Cambridge CHERI-RISCV cores currently requires two separate CHERI toolchains:

**Cambridge CHERI-RISCV 64 bit and 32 bit PURECAP**

* cheribuild (QEMU, LLVM, GDB): https://github.com/CTSRD-CHERI/cheribuild.git (Commit: df913531)

* specs: CHERI-ISA: https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-951.pdf


**Codasip CHERI-RISCV 64 bit and 32 bit ZCHERIPURECAP v0.9.5 spec**

* QEMU: https://github.com/CHERI-Alliance/qemu/tree/codasip-cheri-riscv_v3
(branch: codasip-cheri-riscv-v3, Commit: 2a2e882b)

* LLVM: https://github.com/CHERI-Alliance/llvm-project/tree/codasip-cheri-riscv
(branch: codasip-cheri-riscv, Commit: 1ca584e7)

* GDB: https://github.com/CHERI-Alliance/gdb/tree/codasip-cheri-riscv
(branch: codasip-cheri-riscv, Commit: df929d4d)

* v0.9.5 spec: https://github.com/riscv/riscv-cheri/releases/tag/v0.9.5 

## Patches

### Step1: get patches

Some patches are required for the Codasip qemu and llvm toolchain, together with a script to generate the required builtins. The necessary patches are located in the docker-release orphan branch, so clone that first to `~/zephyrproject/docker-release`.
```
git clone --single-branch -b docker-release https://github.com/CHERI-Alliance/CHERI-zephyr.git docker-release
```

## Cambridge QEMU-CHERI-RISCV:

### Step1: Clone cheribuild repo

clone cheribuild with commit id df913531 to `~/cheribuild`. See https://github.com/CTSRD-CHERI/cheribuild for more information.

```
git clone --no-checkout https://github.com/CTSRD-CHERI/cheribuild.git ~/cheribuild
cd ~/cheribuild
git checkout df913531
```
### Step2: Install dependencies

```
sudo apt update
sudo apt upgrade -y
sudo apt install autoconf automake libtool pkg-config clang bison cmake ninja-build samba flex texinfo time libglib2.0-dev libpixman-1-dev libarchive-dev libarchive-tools libbz2-dev libattr1-dev libcap-ng-dev
sudo apt install libexpat1-dev
sudo apt install libgmp-dev
```
### Step3: Install qemu, llvm, gdb

Build CHERI for freestanding baremetal binaries:
```
cd ~/cheribuild
./cheribuild.py freestanding-cheri-sdk -d -v
```
output when done:
```
Built for target 'freestanding-cheri-sdk'
```
### Step4: Install builtins

Install builtin libraries as required:
```
  ./cheribuild.py -v compiler-rt-builtins-baremetal-riscv32 -d
  ./cheribuild.py -v compiler-rt-builtins-baremetal-riscv32-purecap -d
  ./cheribuild.py -v compiler-rt-builtins-baremetal-riscv64 -d
  ./cheribuild.py -v compiler-rt-builtins-baremetal-riscv64-purecap -d
```

## Codasip QEMU-CHERI-RISCV64 (0.9.5 spec):
### Step1: Clone codasip toolchain repos

Clone qemu to `~/qemu-codasip`, llvm-project to `~/llvm-cheri-codasip`, and gdb to `~/gdb-codasip`. See https://github.com/CHERI-Alliance for more information. Use `git checkout` for a specific commit. The following commit refs have been tested against:

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

### Step2: Install dependencies
```
sudo apt update
sudo apt upgrade -y
sudo apt install autoconf automake libtool pkg-config clang bison cmake ninja-build samba flex texinfo time libglib2.0-dev libpixman-1-dev libarchive-dev libarchive-tools libbz2-dev libattr1-dev libcap-ng-dev libexpat1-dev libgmp-dev python3-pip libgmp-dev libmpfr-dev libmpc-dev
pip install pyelftools
```
### Step3: Install patches

Install patches for 32 bit CHERI-RISCV.

```
cd /qemu-codasip
git apply /zephyrproject/docker-release/support/codasip-qemu-32bit-boot-code_patch.diff

cd /llvm-cheri-codasip
git apply /zephyrproject/docker-release/support/codasip-llvm-32bit-perms-patch.diff;
```

### Step4: Install qemu-codasip
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
### Step5: Install llvm-cheri-codasip

```
cd /llvm-cheri-codasip
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/home/<path>/llvm-cheri-codasip/build -DLLVM_TARGETS_TO_BUILD=RISCV -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-elf" -DLLVM_ENABLE_PROJECTS="clang;lld" -DCMAKE_BUILD_TYPE=Release -GNinja ../llvm/ -DLLVM_PARALLEL_LINK_JOBS=1 -DLLVM_USE_SPLIT_DWARF=ON -DBUILD_SHARED_LIBS=ON -Wno-dev
ninja -j1
```
### Step6: Install builtins

Run the following script to generate the builtins in `~/llvm-builtins-codasip`

```
./zephyrproject/docker-release/support/build_compiler_rt.sh
```
Copy the ones we need to the correct location:

```
mkdir -p ~/llvm-cheri-codasip/build/baremetal/baremetal-riscv32/lib
cp -a ~/llvm-builtins-codasip/rv32imac-ilp32/lib/baremetal/. \
      ~/llvm-cheri-codasip/build/baremetal/baremetal-riscv32/lib/

mkdir -p ~/llvm-cheri-codasip/build/baremetal/baremetal-riscv64/lib
cp -a ~/llvm-builtins-codasip/rv64imac-lp64/lib/baremetal/. \
      ~/llvm-cheri-codasip/build/baremetal/baremetal-riscv64/lib/

mkdir -p ~/llvm-cheri-codasip/build/baremetal/baremetal-riscv32-purecap/lib
cp -a ~/llvm-builtins-codasip/rv32imazcheripurecap-il32pc64/lib/baremetal/. \
      ~/llvm-cheri-codasip/build/baremetal/baremetal-riscv32-purecap/lib/

mkdir -p ~/llvm-cheri-codasip/build/baremetal/baremetal-riscv64-purecap/lib
cp -a ~/llvm-builtins-codasip/rv64imazcheripurecap-l64pc128/lib/baremetal/. \
      ~/llvm-cheri-codasip/build/baremetal/baremetal-riscv64-purecap/lib/
```


### step7: Install gdb-codasip (optional for debugging)

```
cd /gdb-codasip
mkdir build
./configure --target=riscv64-unknown-elf --prefix=/<path>/gdb-codasip/build
make
make install
```

## zephyr (release branch):
### Step1: Clone zephyr (release branch)

Clone to `~/zephyrproject/zephyr`
```
git clone --recurse-submodules --branch <release branch name> https://github.com/CHERI-Alliance/CHERI-zephyr.git ~/zephyrproject/zephyr
```
### Step2: Install dependencies



```
wget https://apt.kitware.com/kitware-archive.sh
sudo bash kitware-archive.sh
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
  make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1
sudo apt install python3-venv  
```

### Step3: Set up environment and install west

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

### Step4: Build Manual clone of CHERI-Zephyr

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

### Step5: Set up tool chain

create `.zephyrrc` file in home directory to default to your desired toolchain.
```
#-------------------------------------
# CUSTOM CMAKE TOOLCHAINS
#-------------------------------------
##llvm-cheri cambridge (cheribuild)
#export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
#export LLVM_CHERI_TOOLCHAIN_PATH=/home/<path>/cheri/output/sdk
#export QEMU_BIN_PATH=/home/<path>/cheri/output/sdk/bin

##llvm-cheri codasip
export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
export LLVM_CHERI_TOOLCHAIN_PATH=/home/<path>/llvm-cheri-codasip/build
export QEMU_BIN_PATH=/home/<path>/qemu-codasip/build
```

### Step6: Build and run sample hello_world

Open a new terminal.

```
cd zephyrproject/zephyr/
source ~/zephyrproject/.venv/bin/activate
source zephyr-env.sh
west build -p always -b qemu_riscv64cheri_zcheripurecap samples/hello_world
west build -t run
```
output:
```
-- west build: running target run
[0/1] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: riscv64cheri
*** Booting Zephyr OS build zephyr ***
Hello World! qemu_riscv64cheri_zcheripurecap
```
`Ctrl+C` to stop QEMU.

## Changing CHERI Toolchains

The toolchain can be invoked by either exporting the environment variables or modifying the `home/.zephyrrc` file. If modifying the `home/.zephyrrc` file always run the following afterwards to update the environment:

```
source zephyr-env.sh
```
Codasip:
```
west build -p always -b qemu_riscv64cheri_zcheripurecap samples/hello_world
west build -t run
```

Cambridge:
```
west build -p always -b qemu_riscv64cheri_purecap samples/hello_world
west build -t run
```

See [README.md](README.md) for the full list of CHERI-specific samples and tests.