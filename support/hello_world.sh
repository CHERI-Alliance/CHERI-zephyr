#!/bin/bash

echo "Building & running Hello, World sample - make sure Zephyr is built, and you have sourced the CPU type shell script"

cd zephyr
west build -p always -b qemu_riscv64cheri_zcheripurecap samples/hello_world
west build -t run