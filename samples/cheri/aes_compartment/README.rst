.. zephyr:code-sample:: aes_compartment
   :name: AES Compartmentalization demo

   Demonstrates how to compartmentalize libraries using CHERI hardware isolation

Overview
********

This is a 'manual' Proof of Concept (PoC) sample to show how CHERI compartments can be used to replace PMP for user threads and software compartmentalization.

The main.c file prepares interface tables (which allow data and explicitly specified capabilities to be passed from an application running in kernel space to a compartment running in user space), manually sets up the compartment bounds (as defined in the manually edited linker script in the ``linker`` subdirectory), and creates threads to execute entry point functions.  The entrypoint functions are a thin layer between the main program and the largely unedited, compartmentalized library code; they allow you to interact with the library and input/output data.  Each compartment is initialized with its own stack.

Properly resized compartments should be unable to 'reach out' of their compartment spaces nor should they be able to 'reach in' to other bounded compartments, unless they have been given explicit permission by the kernel via a capability.  This allows for workflows where one or more untrusted libraries can be used by an application without fear that if the library has malicious code (say, due to a supply chain attack) or later becomes compromised at runtime, it will be able to compromise the application itself or attack other libraries.

For example, we envision this sample as being of interest to IoT developers who might compartmentalize netcode, sensor packages or even controls for cyberphysical systems.  In contrast to RISC-V's Physical Memory Protection(PMP) feature, one can create an unlimited number of compartments by virtue of the CHERI memory model.  There are also fewer context switches involved moving between them.

We demonstrate this functionality by compartmentalizing a freely available AES library, tiny-AES to perform compartmentalized encryption and decryption.  The tiny-AES library is statically compiled with its own simple Makefile by the standard zephyr build system and then (statically) linked into the zephyr.elf system image.

Structure Overview
==================

.. code-block::
    ├── aeslib							The AES library root
    │   ├── include
    │   ├── Makefile
    │   └── src
    ├── linker
    │   └── linker_compartments.ld		The custom linker script that helps organize elf sections
    │										into contiguous regions
    │
    ├── CMakeLists.txt					The standard zephyr CMakeLists.txt file
    ├── README.rst						This file
    ├── prj.conf
    ├── sample.yaml
    └── src
        ├── compartment_entry.h
        ├── compartment_entry.c			The compartment's entrypoint code
        ├── compartment_interface.h		Definition of compartment interface
        ├── attack_entry.h
        ├── attack_interface.h			Definition of the attack compartment interface
        ├── attack_entry.c				The attack compartment (used in some tests)'s entrypoint
        ├── inline_funcs.h
        └── main.c						The main application entrypoint

Limitations
===========
* Only tested with ``qemu_riscv64cheri_zcheripurecap`` and ``qemu_riscv64cheri_sma_zcheripurecap``
* Requires a library-level adjustment to the global linker setup so as not to
  greedily pull standalone library ELF sections into standard places.
* Some code will need to be rewritten in inline assembly due to the way that global variables are always stored in the global captable section, outside of any compartment.

How to run?
===========
* To build / run the example on the qemu_riscv64cheri_zcheripurecap board (runs test 2 by default)::

    west build -p always -b qemu_riscv64cheri_zcheripurecap samples/cheri/aes_compartment
    west -v build -t run

* To run a specific test (replace test1 with test1-6)::

    west build -b qemu_riscv64cheri_zcheripurecap samples/cheri/aes_compartment -t run -T samples.cheri.aes_compartment.test1

* To run all tests::

    scripts/twister -T samples/cheri/aes_compartment -v

Design
======
* Based on the 'external static library' zephyr example, with heavy modifications

How to Compartmentalize your own libraries
==========================================
* All library code is siloed outside the 'src' directory
* Prepare your library / libraries to be statically compiled
	* You may need to alter the makefile / build system to work for zephyr/CHERI
	* Sync with the sample's top-level CMakeLists.txt file
* Most of your code can exist unedited like tinyaes in our aeslib directory
	* However, any global data arrays (e.g. sbox, rsbox and rcon) will need to be rewritten
	* This must be done in assembly (see tables.s)
	* Offsets from accessor functions to their corresponding tables will need offset values adjust
* The linker script will have to be adjusted manually to account for your library's build artifacts
	* Or to account for more than one library / compartment
* The global riscv linker script will also have to be tweaked based on the names of your build artifacts

Future directions
=================
* Automating the integration with third party libraries -- no manual work or offset calculations
* Support for 32bit CHERI ISAs



Windows Note
************

To use this sample on a Windows host operating system, GNU Make needs to be in
your path. This can be setup using chocolatey or by manually installing it.

Chocolatey Method
=================

Install make using the following command:

.. code-block:: bash

   choco install make

Once installed, build the application as normal.

Manual Install Method
=====================

The pre-built make application can be downloaded from
https://gnuwin32.sourceforge.net/packages/make.htm by either getting both the
``Binaries`` and ``Dependencies`` and extracting them to the same folder, or
getting the ``Complete package`` setup. Once installed and in the path, build
the application as normal.
