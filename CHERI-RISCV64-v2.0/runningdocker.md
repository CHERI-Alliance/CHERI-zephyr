
# Running docker for CHERI-RISCV64-v2.0

## To build docker
 Firstly clone the docker-release branch to **zephyrproject/docker-release**
 (assumes zephyr is cloned to zephyrproject/zephyr).
 
## zephyr docker 
Runs all twister tests - tests/samples for qemu_riscv64/qemu_riscv32 with the Zephyr SDK.
### To build: (takes one hour)
```
cd zephyrproject
docker build --no-cache -t zephyrimage -f docker-release/CHERI-RISCV64-v2.0/zephyrdockerfile .
```
### To build wih buildx:
```
docker buildx build -t zephyrimage -f docker-release/CHERI-RISCV64-v2.0/cheridockerfile --load .
```
### To run
```
docker run -it zephyrimage bash
```
### Running tests
Run the following tests. Run the tests after the the docker container has been created.
* python3 zephyr/scripts/twister -p qemu_riscv64 -T zephyr/tests 
* python3 zephyr/scripts/twister -p qemu_riscv32 -T zephyr/tests
* python3 zephyr/scripts/twister -p qemu_riscv64 -T zephyr/samples
* python3 zephyr/scripts/twister -p qemu_riscv32 -T zephyr/samples

The reason for this is that some intermittent time out errors can occur with the following sample tests, and prevent the docker from being built:
* /zephyr/samples/subsys/testsuite/pytest/shell/sample.harness.shell.vt100_colors_off 
* /zephyr/samples/subsys/testsuite/pytest/shell/sample.harness.shell
* /zephyr/samples/sensor/sensor_shell/sample.sensor.shell.pytest 

### Expected outputs
**qemu_riscv64 -T zephyr/tests**
* Total complete:  684/ 684  100%  built (not run):   26, filtered: 1867, failed:    0, error:    0
* INFO - 582 of 608 executed test configurations passed (95.72%), 26 built (not run), 0 failed, 0 errored

**qemu_riscv32-T zephyr/tests**
* Total complete:  683/ 683  100%  built (not run):   26, filtered: 1863, failed:    0, error:    0
* INFO - 586 of 612 executed test configurations passed (95.75%), 26 built (not run), 0 failed, 0 errored

**qemu_riscv64-T zephyr/samples**
* Total complete:  172/ 172  100%  built (not run):   17, filtered:  886, failed:    0, error:    0 `-possible intermittent 1 to 3 time out errors`
* INFO - 84 of 101 executed test configurations passed (83.17%), 17 built (not run), 0 failed, 0 errored `-possible intermittent 1 to 3 time out errors`

**qemu_riscv32-T zephyr/samples**
* Total complete:  173/ 173  100%  built (not run):   17, filtered:  885, failed:    0, error:    0
* INFO - 85 of 102 executed test configurations passed (83.33%), 17 built (not run), 0 failed, 0 errored

## CHERI docker 
Runs a subset of twister tests - tests/samples for qemu_riscv64cheri with cheribuild/codasip SDK.
### To build wih all features: (takes several hours)
```
cd zephyrproject
docker build --no-cache -t cherizephyrimage -f docker-release/CHERI-RISCV64-v2.0/cheridockerfile .
```
### To select features:
```
docker build --no-cache --build-arg FEATURE_CHERIBUILD=false --build-arg FEATURE_CODASIP=true -t cherizephyrimage -f docker-release/CHERI-RISCV64-v2.0/cheridockerfile .
```
### To build wih buildx:
```
docker buildx build -t cherizephyrimage -f docker-release/CHERI-RISCV64-v2.0/cheridockerfile --load .
docker buildx build --build-arg FEATURE_CHERIBUILD=false --build-arg FEATURE_CODASIP=true -t cherizephyrimage -f docker-release/CHERI-RISCV64-v2.0/cheridockerfile --load .
```
### To run
```
docker run -it cherizephyrimage bash
```
### Running tests
In docker (for codasip core):
```
 cd zephyrproject
 source ~/zephyrproject/.venv/bin/activate
 export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
 export LLVM_CHERI_TOOLCHAIN_PATH=/home/builder/llvm-cheri-codasip/build
 export QEMU_BIN_PATH=/home/builder/qemu-codasip/build
 cd zephyr
```
Run an app in QEMU:
```
 west build -p always -b qemu_riscv64cheri samples/hello_world
 west build -p always -b qemu_riscv64cheri samples/synchronization
 west build -p always -b qemu_riscv64cheri samples/philosophers
 west build -p always -b qemu_riscv64cheri samples/basic/sys_heap

 west build -p always -b qemu_riscv64cheri_smp samples/hello_world
 west build -p always -b qemu_riscv64cheri_smp samples/synchronization
 west build -p always -b qemu_riscv64cheri_smp samples/philosophers
 west build -p always -b qemu_riscv64cheri_smp samples/basic/sys_heap

 west build -p always -b qemu_riscv64cheri_zcheripurecap samples/hello_world
 west build -p always -b qemu_riscv64cheri_zcheripurecap samples/synchronization
 west build -p always -b qemu_riscv64cheri_zcheripurecap samples/philosophers
 west build -p always -b qemu_riscv64cheri_zcheripurecap samples/basic/sys_heap

 west build -p always -b qemu_riscv64cheri_smp_zcheripurecap samples/hello_world
 west build -p always -b qemu_riscv64cheri_smp_zcheripurecap samples/synchronization
 west build -p always -b qemu_riscv64cheri_smp_zcheripurecap samples/philosophers
 west build -p always -b qemu_riscv64cheri_smp_zcheripurecap samples/basic/sys_heap

 west build -t run
 ```
 Run a twister test:  expect to all pass
 ```
 python3 scripts/twister -p qemu_riscv64cheri -T samples/hello_world
 python3 scripts/twister -p qemu_riscv64cheri -T samples/synchronization
 python3 scripts/twister -p qemu_riscv64cheri -T samples/philosophers
 python3 scripts/twister -p qemu_riscv64cheri -T samples/basic/sys_heap

 python3 scripts/twister -p qemu_riscv64cheri_smp -T samples/hello_world
 python3 scripts/twister -p qemu_riscv64cheri_smp -T samples/synchronization
 python3 scripts/twister -p qemu_riscv64cheri_smp -T samples/philosophers
 python3 scripts/twister -p qemu_riscv64cheri_smp -T samples/basic/sys_heap

 python3 scripts/twister -p qemu_riscv64cheri_zcheripurecap -T samples/hello_world
 python3 scripts/twister -p qemu_riscv64cheri_zcheripurecap -T samples/synchronization
 python3 scripts/twister -p qemu_riscv64cheri_zcheripurecap -T samples/philosophers
 python3 scripts/twister -p qemu_riscv64cheri_zcheripurecap -T samples/basic/sys_heap

 python3 scripts/twister -p qemu_riscv64cheri_smp_zcheripurecap -T samples/hello_world
 python3 scripts/twister -p qemu_riscv64cheri_smp_zcheripurecap -T samples/synchronization
 python3 scripts/twister -p qemu_riscv64cheri_smp_zcheripurecap -T samples/philosophers
 python3 scripts/twister -p qemu_riscv64cheri_smp_zcheripurecap -T samples/basic/sys_heap
 ```
 Run all twister tests -  There is currently no expectation to pass all tests
 ```
 python3 scripts/twister -p qemu_riscv64cheri -T samples/ 
 python3 scripts/twister -p qemu_riscv64cheri -T tests/

 python3 scripts/twister -p qemu_riscv64cheri_zcheripurecap -T samples/
 python3 scripts/twister -p qemu_riscv64cheri_zcheripurecap -T tests/

 ```
Ctrl+C to stop QEMU

In docker (for cambs cheribuild):
```
 cd zephyrproject
 source ~/zephyrproject/.venv/bin/activate
 export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
 export LLVM_CHERI_TOOLCHAIN_PATH=/home/builder/cheri/output/sdk
 export QEMU_BIN_PATH=/home/builder/cheri/output/sdk/bin
 cd zephyr
 ```
Run an app in QEMU:
```
 west build -p always -b qemu_riscv64cheri samples/hello_world
 west build -p always -b qemu_riscv64cheri samples/synchronization
 west build -p always -b qemu_riscv64cheri samples/philosophers
 west build -p always -b qemu_riscv64cheri samples/basic/sys_heap

 west build -p always -b qemu_riscv64cheri_smp samples/hello_world
 west build -p always -b qemu_riscv64cheri_smp samples/synchronization
 west build -p always -b qemu_riscv64cheri_smp samples/philosophers
 west build -p always -b qemu_riscv64cheri_smp samples/basic/sys_heap

 west build -p always -b qemu_riscv64cheri_purecap samples/hello_world
 west build -p always -b qemu_riscv64cheri_purecap samples/synchronization
 west build -p always -b qemu_riscv64cheri_purecap samples/philosophers
 west build -p always -b qemu_riscv64cheri_purecap samples/basic/sys_heap

 west build -p always -b qemu_riscv64cheri_smp_purecap samples/hello_world
 west build -p always -b qemu_riscv64cheri_smp_purecap samples/synchronization
 west build -p always -b qemu_riscv64cheri_smp_purecap samples/philosophers
 west build -p always -b qemu_riscv64cheri_smp_purecap samples/basic/sys_heap

 west build -t run
 ```
Run a twister test:  expect to all pass
```
 python3 scripts/twister -p qemu_riscv64cheri -T samples/hello_world
 python3 scripts/twister -p qemu_riscv64cheri -T samples/synchronization
 python3 scripts/twister -p qemu_riscv64cheri -T samples/philosophers
 python3 scripts/twister -p qemu_riscv64cheri -T samples/basic/sys_heap

 python3 scripts/twister -p qemu_riscv64cheri_smp -T samples/hello_world
 python3 scripts/twister -p qemu_riscv64cheri_smp -T samples/synchronization
 python3 scripts/twister -p qemu_riscv64cheri_smp -T samples/philosophers
 python3 scripts/twister -p qemu_riscv64cheri_smp -T samples/basic/sys_heap

 python3 scripts/twister -p qemu_riscv64cheri_purecap -T samples/hello_world
 python3 scripts/twister -p qemu_riscv64cheri_purecap -T samples/synchronization
 python3 scripts/twister -p qemu_riscv64cheri_purecap -T samples/philosophers
 python3 scripts/twister -p qemu_riscv64cheri_purecap -T samples/basic/sys_heap

 python3 scripts/twister -p qemu_riscv64cheri_smp_purecap -T samples/hello_world
 python3 scripts/twister -p qemu_riscv64cheri_smp_purecap -T samples/synchronization
 python3 scripts/twister -p qemu_riscv64cheri_smp_purecap -T samples/philosophers
 python3 scripts/twister -p qemu_riscv64cheri_smp_purecap -T samples/basic/sys_heap
```
Run all twister tests -  There is currently no expectation to pass all tests
 ```
 python3 scripts/twister -p qemu_riscv64cheri -T samples/ 
 python3 scripts/twister -p qemu_riscv64cheri -T tests/

 python3 scripts/twister -p qemu_riscv64cheri_purecap -T samples/
 python3 scripts/twister -p qemu_riscv64cheri_purecap -T tests/

 ```
`Ctrl+C` to stop QEMU

`exit` to stop docker




