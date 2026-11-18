.. zephyr:code-sample:: hobgoblin_uarts
   :name: Hobgoblin Uarts

   Print and read characters using hobgoblin uart

Overview
********

A simple sample that can be used to test either the uartlite
or the NS16550 uart on the hobgoblin platform using polling
input characters or interrupts.

Building and Running
********************

This application can be built and executed on the Codasip hobgoblin
physical platform as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/cheri/hobgoblin/hobgoblin_uarts
   :host-os: unix
   :board: hobgoblin_riscv64cheri
   :goals: build
   :compact:

Polling and NS16550 UART are selected as default.
For selecting interrupts and uartlite in a west build add as follows:

west build -p always -b hobgoblin_riscv64cheri_zcheripurecap \
samples/cheri/hobgoblin/hobgoblin_uarts/ -DCHERI_TEST_UART_INTERRUPTS=ON \
-DDTC_OVERLAY_FILE=${ZEPHYR_BASE}/boards/codasip/hobgoblin_riscv64cheri/uart1.overlay

Sample Output
=============

.. code-block:: console

*** Booting Zephyr OS build f06d6557e0c4 ***
UART test for hobgoblin_riscv64cheri/hobgoblin_x730_mp4_riscv64cheri
Enter some characters for polling

RX: h
RX: e
RX: l
RX: l
RX: o

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
