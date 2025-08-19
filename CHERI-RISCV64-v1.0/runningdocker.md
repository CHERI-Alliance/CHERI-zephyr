
# Running docker for CHERI-RISCV64-v1.0

## To build docker
 Firstly clone the docker-release branch to **project_name/docker-release**
 (assumes zephyr is cloned to project_name/zephyr).
 
## zephyr docker 
Runs all twister tests - tests/samples for qemu_riscv64/qemu_riscv32 with zephyr sdk.
### To build: (takes one hour)
```
cd zephyrproject
docker build --no-cache -t zephyrimage -f docker-release/CHERI-RISCV64-v1.0/zephyrdockerfile .
```
### To build wih buildx:
```
docker buildx build -t zephyrimage -f docker-release/CHERI-RISCV64-v1.0/cheridockerfile --load .
```
### To run
```
docker run -it zephyrimage bash
```
## CHERI docker 
Runs a subset of twister tests - tests/samples for qemu_riscv64 with cheribuild/codasip sdk.
### To build wih all features: (takes several hours)
```
cd zephyrproject
docker build --no-cache -t cherizephyrimage -f docker-release/CHERI-RISCV64-v1.0/cheridockerfile .
```
### To select features:
```
docker build --no-cache --build-arg FEATURE_CHERIBUILD=false --build-arg FEATURE_CODASIP=true -t cherizephyrimage -f docker-release/CHERI-RISCV64-v1.0/cheridockerfile .
```
### To build wih buildx:
```
docker buildx build -t cherizephyrimage -f docker-release/CHERI-RISCV64-v1.0/cheridockerfile --load .
docker buildx build --build-arg FEATURE_CHERIBUILD=false --build-arg FEATURE_CODASIP=true -t cherizephyrimage -f docker-release/CHERI-RISCV64-v1.0/cheridockerfile --load .
```
### To run
```
docker run -it cherizephyrimage bash
```

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
 west build -p always -b qemu_riscv64cheri_zcheripurecap samples/hello_world
 west build -t run
 ```
 Run a twister test:
 ```
 python3 scripts/twister -p qemu_riscv64 -T samples/hello_world
 python3 scripts/twister -p qemu_riscv64cheri_zcheripurecap -T samples/hello_world
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
 west build -p always -b qemu_riscv64cheri samples/hello_world
 west build -p always -b qemu_riscv64cheri_purecap samples/hello_world
 west build -t run
 ```
Run a twister test:
```
 python3 scripts/twister -p qemu_riscv64 -T samples/hello_world
 python3 scripts/twister -p qemu_riscv64cheri_purecap -T samples/hello_world
```
Ctrl+C to stop QEMU




