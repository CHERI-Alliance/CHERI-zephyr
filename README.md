# CHERI-Zephyr++

*The Zephyr project and this project are released under an Apache 2.0 LICENSE. This work is/was funded by the Digital Security by Design (DSbD) Programme, a University of Birmingham EPSRC IAA grant, and an Innovate UK contract for innovation.*

This release is CHERI-Zephyr based on an upstream version of Zephyr.

CHERI-Zephyr along with the CHERI-based tool chains can be run using Docker following the below instructions, or installed manually per the [manual installation instructions](ManualInstall.md).

Limitations: This version currently provides CHERI support to run the applications listed. It does not yet provide full CHERI support to every Zephyr feature.

# CHERI Support

Current CHERI support is provided for QEMU system emulation of the following CHERI RISC-V machines:

* Codasip CHERI-RISCV64 ZCHERIPURECAP
* Codasip CHERI-RISCV32 ZCHERIPURECAP
* Cambridge CHERI-RISCV64 PURECAP
* Cambridge CHERI-RISCV32 PURECAP

# Building and running docker

The docker image includes everything you need to run QEMU tests for the above CHERI RISCV machines including LLVM, QEMU, and GDB for both the Cambridge (cheribuild) and Codasip versions.

The following assumes that you are in a working directory called `zephyrproject`.
The process has 3 Stages:
1. Stage 1: builds CHERI-llvm (sources and artifacts are in the Docker image)
    * You can optionally skip this build step and download pre-built debs.
2. Stage 2: builds Zephyr west (sources external to Docker image: git and Stage 1)
3. Stage 3: initializes the environment for running tests (This stage is sourced from image in Stage 2)

## Clone GIT repos
Firstly, clone the docker-release branch to `docker-release`.

```
git clone --single-branch -b docker-release https://github.com/CHERI-Alliance/CHERI-zephyr.git docker-release
```
Then, clone Zephyr into the working directory as well (required from STAGE 2: cheri-zephyr-west):

```
git clone --recurse-submodules --branch <BRANCH_NAME_HERE> https://github.com/CHERI-Alliance/CHERI-zephyr.git zephyr
```

where `<BRANCH_NAME_HERE>` is the desired release, e.g. `vx.x.x-cheri-branch`.

## Building the dev environment
You might want to add `--no-cache` on rebuilds after substantial changes.


### Stage 1: Build CHERI-llvm
Note: building the llvm image from scratch (from an Ubuntu base image) can take more than 1 hour.  For alternatives, see below.

```
docker build -t cheri-llvm -f docker-release/cheri-zephyr-llvm.Dockerfile .
```

#### Option: Select certain toolchains:
Use the `--build-arg` flag to only build certain toolchains, eg:
```
docker build --build-arg FEATURE_CHERIBUILD=false --build-arg FEATURE_CODASIP=true -t cheri-llvm -f docker-release/cheri-zephyr-llvm.Dockerfile .
```
#### Option: Use precompiled toolchain (debs)
Alternatively, you can download precompiled custom debs (see: https://github.com/cheri-zephyr-project/custom-debs/releases/tag/custom-llvm-debs-0.2) into the root folder, and then use a build argument for stage 2 (see below):

```
mkdir custom-debs
cd custom debs
gh release download custom-llvm-debs-0.2 --repo cheri-zephyr-project/custom-debs --pattern '*.deb'
```

### Stage 2: Build Zephyr west
This step is moderately quick, starts from a Ubuntu base image and requires Stage 1 and a clone of the zephyr repo in a 'zephyr' subdir at the same level as docker-release.

```
docker build -t cheri-zephyr-west -f docker-release/cheri-zephyr-west.Dockerfile .
```

#### Option: Use precompiled toolchain (debs)
If you have downloaded the custom debs, then run this instead:

```
docker build -t cheri-zephyr-west -f docker-release/cheri-zephyr-west-prebuilt-debs.Dockerfile .
```

### Stage 2b: Live Zephyr Mount (Optional):
If you want to work on a clone of Zephyr mounted into the container, proceed as follows - otherwise you can instead go to stage 3.

Mount into Docker and run a shell:

```
docker run -v ./zephyr:/home/user/zephyrproject/zephyr -it cheri-zephyr-west bash
```

Now, in the docker shell, run `./build-zephyr.sh`. Now, you should be able to proceed with running tests as per below (skip stage 3). Note however that this downloads a substantial amount of data into the container, which will not persist. So be careful about closing the interactive container while debugging.

You can, if you want to keep the main Zephyr clone clean, checkout a separate copy of the GIT and mount that:

```
git clone --recurse-submodules --branch <BRANCH_NAME_HERE> https://github.com/CHERI-Alliance/CHERI-zephyr.git zephyr-work
docker run -v ./zephyr-work:/home/user/zephyrproject/zephyr -it cheri-zephyr-west bash
```

### Stage 3: Build the dev environment

This should take less than 10 minutes.

*Note:* This will copy and build the `zephyr` folder into the container. If you want to work with a mounted folder instead, refer to stage 2b, just above.

```
docker build -t cheri-zephyr-devenv -f docker-release/cheri-zephyr-devenv.Dockerfile .
```

### Running the dev environment

```
docker run -it cheri-zephyr-devenv bash
```
The following is triggered automatically from ./bashrc to default select the Codasip toolchain.

```
. ./codasip.sh
```
## CHERI board support

The following CHERI QEMU boards are supported:

Codasip/Cambridge - build RISCV:
* qemu_riscv64cheri
* qemu_riscv64cheri_smp
* qemu_riscv32cheri
* qemu_riscv32cheri_smp

Codasip - build RISCV with CHERI Capabilities (zcheripurecap only):
* qemu_riscv64cheri_zcheripurecap
* qemu_riscv64cheri_smp_zcheripurecap
* qemu_riscv32cheri_zcheripurecap
* qemu_riscv32cheri_smp_zcheripurecap

Cambridge (cheribuild) - build RISCV with CHERI Capabilities (purecap only):
* qemu_riscv64cheri_purecap
* qemu_riscv64cheri_smp_purecap
* qemu_riscv32cheri_purecap
* qemu_riscv32cheri_smp_purecap

The following CHERI physical hardware platforms are supported:

Codasip - hobgoblin platform based on the VCU118 FPGA board:
* hobgoblin_riscv32cheri
* hobgoblin_riscv32cheri_zcheripurecap
* hobgoblin_riscv64cheri
* hobgoblin_riscv64cheri_smp (4 cores)
* hobgoblin_riscv64cheri_zcheripurecap
* hobgoblin_riscv64cheri_smp_zcheripurecap (4 cores)



## Running tests

You can either build and run individual tests using `west build`, or run various test scripts which use `twister` as described below.

### Statistics summary - for all CHERI-RISCV

To get a summary report of all the kernel specific twister tests that pass for all CHERI architectures you can run the following: (takes at least 1hr)
```
./run_stats.sh
```
This runs twister tests including arch, kernel and added cheri tests for the supported 32 and 64 bit CHERI-RISCV architectures, and prints out a summary of the results:
```
=====================================
 CHERI‑RISC‑V Twister Test Results
 arch + kernel + cheri
=====================================
[CODASIP_32_BIT]
  Test Configurations: 200/247 (80.97%)
  Test Cases:   1946/2275 (85.54%)
[CODASIP_64_BIT]
  Test Configurations: 179/245 (73.06%)
  Test Cases:   1939/2284 (84.89%)
[CHERIBUILD_32_BIT]
  Test Configurations: 170/247 (68.83%)
  Test Cases:   1701/2329 (73.04%)
[CHERIBUILD_64_BIT]
  Test Configurations: 174/245 (71.02%)
  Test Cases:   1923/2284 (84.19%)
```

### Basic twister samples

To run a basic set of twister samples for the CHERI architectures you can run the scripts below. These tests run the following samples:

* Zephyr apps: hello_world, synchronization, philosophers, basic/sys_heap
* CHERI-specific: buffer_overflow, mem_blocks_overflow, mem_slab_overflow, stack_overflow

Codasip:
```
. ./tests-codasip-twister-basic.sh
```
Cambridge:
```
. ./tests-cheribuild-twister-basic.sh
```

### Building with west

Samples and tests can be built and run individually using west.

Codasip:
```
. ./codasip.sh
west build -p always -b qemu_riscv64cheri_zcheripurecap samples/hello_world
west build -t run
```

Cambridge:
```
. ./cambridge.sh
west build -p always -b qemu_riscv64cheri_purecap samples/hello_world
west build -t run
```

`Ctrl+C` to stop QEMU, `exit` to stop docker

### CHERI-specific tests and samples

Below is a list of CHERI-specific tests and samples that have been added to the zephyr code base, and can be built and run using west.

tests/cheri:
* lib/mem_blocks - testing CHERI modified mem_blocks api and bounds verification
* lib/heap_sys_heap - testing CHERI modified memory allocator api
* lib/heap - testing CHERI modified memory allocator api
* lib/heap_align - testing CHERI modified memory allocator api
* lib/multi_heap - testing CHERI modified memory allocator api
* lib/c_lib - testing CHERI modified memory functions of Zephyr's minimal libc library
* kernel/mem_slab - testing CHERI modified mem_slab api and bounds verification
* kernel/stack - testing CHERI modified stack allocation and bounds verification
* kernel/mem_heap - testing CHERI modified memory allocator api

samples/cheri:
* buffer_overflow - CHERI preventing basic overflow
* mem_blocks_overflow - CHERI preventing individual block overflow
* mem_slab_overflow - CHERI preventing individual block overflow
* stack_overflow - CHERI preventing single thread stack overflow
* sys_heap_overflow - CHERI preventing heap overflow
* sys_multi_heap_overflow - CHERI preventing heap overflow
* k_heap_overflow - CHERI preventing heap overflow
* k_malloc_overflow - CHERI preventing heap overflow

## Feature branch Modbus demo

To run a modbus demo on the Codasip CHERI-RISCV cores you will need to clone the feature branch `feature/0.3.0/demonstration` instead of the release branch in to `~/zephyrproject`. Then re-run the docker builds for stage 2 and stage 3. This feature version has been patched with a second QEMU UART1. This patch hence also adds a DTSI for this QEMU variant and makes NS16550 driver IRQs compatible to CHERI builds. Currently the demo can be run in non-Capability mode as follows:

from `zephyrproject` within the running container:

* 64-bit
```
./build-run-demo.sh
```
* 32-bit

```
./build-run-demo-32.sh
```

Expected output example snippet with interleaved transactions:

```
CLIENT (left)                                 | SERVER (right)
----------------------------------------------|--------------------------------
....                                          | ....
D: Start frame transmission                   | D: Start frame transmission
D: mbc_send_cmd: unit ID = 01, fc = 01        | D: Server RX handler 0x8000d4f0
I: Coils state 0x07                           | I: Coil read, addr 0, 0
D: uart_buf                                   | I: Coil read, addr 1, 0
D: 01 0f 00 00 00 03 01 00 |........          | I: Coil read, addr 2, 0
D: 8f 57                   |.W                | D: uart_buf
....                                          | ....
```

## Feature branch CHERI compartment userspace demo

This is a branch of CHERI-Zephyr with a developmental proof-of-concept CHERI compartmentalisation of userspace threads, which replaces the need for the traditional PMP. A sample under samples/cheri/aes_compartment runs an AES encryption library inside a CHERI compartment. Examples show CHERI hardware exceptions when trying to access outside of the compartment or from one compartment to another unless given explicit access. Note this is a proof of concept and should be used with caution. See the sample README file for more details.

## Feature branch recovery analysis

This is an orphan branch with a developmental proof-of-concept approach to CHERI hardware exception recovery using automated static analysis of thread dependency and recovery. Future C code generation from impact analysis output can be acted on from within Zephyr when a thread crashes. The branch here focuses on static analysis techniques.

## Non-CHERIfied Zephyr

### Building Zephyr image

If you want to run `non-cheri` tests with the standard Zephyr tool chain you can build a docker image as per below. This uses a prebuilt container of the Zephyr tools compatible with this Zephyr version. This will also copy and build the `zephyr` folder into the container. This is relatively quick.

```
docker build -t zephyr -f docker-release/Zephyr-sdk.Dockerfile .
```

### Running the  Zephyr image

```
docker run -it zephyr bash
```

### Running tests

You can then run non-cheri standard RISCV tests as follows: (To run all the tests can take a long time)

```
python3 zephyr/scripts/twister -p qemu_riscv64 -T zephyr/tests
python3 zephyr/scripts/twister -p qemu_riscv32 -T zephyr/tests
python3 zephyr/scripts/twister -p qemu_riscv64 -T zephyr/samples
python3 zephyr/scripts/twister -p qemu_riscv32 -T zephyr/samples
```

Note: it is common that sometimes intermittent time out errors can occur with the following sample tests, (depending on speed of machine etc). Refer to zephyr documentation for twister commands to tweak time-out settings.

* /zephyr/samples/subsys/testsuite/pytest/shell/sample.harness.shell.vt100_colors_off
* /zephyr/samples/subsys/testsuite/pytest/shell/sample.harness.shell
* /zephyr/samples/sensor/sensor_shell/sample.sensor.shell.pytest