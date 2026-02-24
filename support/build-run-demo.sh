#!/bin/bash
set -e  # exit immediately if a command exits with a non-zero status

DEMO_DIR=/home/user/zephyrproject/demo
TOOL_DIR=/opt/cheri

source /home/user/zephyrproject/codasip.sh

# Preparations
rm -rf $DEMO_DIR
mkdir $DEMO_DIR

cd /home/user/zephyrproject/zephyr

git status > $DEMO_DIR/git-info.txt
echo "=====" >> $DEMO_DIR/git-info.txt
git log -n1 >> $DEMO_DIR/git-info.txt

echo
echo "=== Building CHERI modbus server ==="
echo

west build -p always -b qemu_riscv64cheri samples/subsys/modbus/rtu_server_cheri
cp /home/user/zephyrproject/zephyr/build/zephyr/zephyr.elf $DEMO_DIR/server.elf

echo 
echo "=== Done ==="
echo

echo
echo "=== Building CHERI modbus client ==="
echo

west build -p always -b qemu_riscv64cheri samples/subsys/modbus/rtu_client_cheri
cp /home/user/zephyrproject/zephyr/build/zephyr/zephyr.elf $DEMO_DIR/client.elf

echo 
echo "=== Done ==="
echo

cd $DEMO_DIR

echo 
echo "=== Setting up pipes & QEMU ==="
echo

rm -f null-cable-*
mkfifo null-cable-1.{in,out}
ln null-cable-1.in null-cable-2.out
ln null-cable-1.out null-cable-2.in


tmux new-session -d 'sleep 1;'"$TOOL_DIR"'/qemu-codasip-patched/build/qemu-system-riscv64cheri -nographic -machine virt -bios none -m 256 -net none -pidfile qemu.pid -chardev stdio,id=con,mux=on -serial chardev:con -mon chardev=con,mode=readline -serial pipe:null-cable-1 -icount shift=6,align=off,sleep=on -rtc clock=vm -kernel client.elf' \; split-window -h 'sleep 1.5;'"$TOOL_DIR"'/qemu-codasip-patched/build/qemu-system-riscv64cheri -nographic -machine virt -bios none -m 256 -net none -pidfile qemu2.pid -chardev stdio,id=con,mux=on -serial chardev:con -mon chardev=con,mode=readline -serial pipe:null-cable-2 -icount shift=6,align=off,sleep=on -rtc clock=vm -kernel server.elf' \; attach-session
