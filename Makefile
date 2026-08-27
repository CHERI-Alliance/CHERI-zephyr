# Makefile for cloning, building, and analyzing real-world Zephyr applications
# with the Zephyr static crash analyzer.
#
# Each project gets its own directory under this repo:
#   coupling-test/<project>/         -- project directory
#     ├── source/                     -- project git checkout
#     ├── .venv/                      -- per-project Python venv
#     ├── zephyr/                     -- zephyr (from west update)
#     ├── modules/                    -- west modules
#     ├── .cloned                     -- flag file (touched after successful clone)
#     └── build/                      -- build output
#
# ZEPHYR_BASE must NOT be set globally — each project manages its own zephyr.
#
# Usage:
#   make clone          - clone all projects
#   make build          - build all projects
#   make analyze        - run analyzer on all projects
#   make clone-zswatch  - clone just ZSWatch
#   make build-zswatch  - build just ZSWatch
#   etc.
#
# Prerequisites (Docker — recommended):
#   ./make.sh <target>           - runs make inside the coupling-test container
#
# Prerequisites (native):
#   - Zephyr SDK at $(ZEPHYR_SDK) (default: /zephyr-sdk-1.0.1)
#   - Crash analyzer built (or set ANALYZER)

ZEPHYR_SDK ?= /zephyr-sdk-1.0.1
ANALYZER ?= $(CURDIR)/analyzer/build/zephyr-crash-analyzer-static

# Base directory for all test projects
BASE_DIR := $(CURDIR)

SHELL := /bin/bash

# Per-project venvs
ZSWATCH_VENV := $(BASE_DIR)/zswatch/.venv
ZMK_VENV := $(BASE_DIR)/zmk/.venv
QUADCOPTER_VENV := $(BASE_DIR)/quadcopter/.venv
USP_ZEPHYR_VENV := $(BASE_DIR)/usp-zephyr/.venv
CANNECTIVITY_VENV := $(BASE_DIR)/cannectivity/.venv
ECFW_VENV := $(BASE_DIR)/ecfw/.venv
OPENDECK_VENV := $(BASE_DIR)/opendeck/.venv
SPINNER_VENV := $(BASE_DIR)/spinner/.venv
GRBL_VENV := $(BASE_DIR)/grbl/.venv
GOLIOTH_VENV := $(BASE_DIR)/golioth/.venv
LIBRESOLAR_VENV := $(BASE_DIR)/libresolar/.venv
SOF_VENV := $(BASE_DIR)/sof/.venv
MICROROS_VENV := $(BASE_DIR)/microros/.venv
FINDMYCAT_VENV := $(BASE_DIR)/findmycat/.venv
LINARO_VENV := $(BASE_DIR)/linaro/.venv
AKIRAOS_VENV := $(BASE_DIR)/akiraos/.venv
ASSETTRACKER_VENV := $(BASE_DIR)/assettracker/.venv
THINGSET_VENV := $(BASE_DIR)/thingset/.venv
BACNET_VENV := $(BASE_DIR)/bacnet/.venv
LIBCSP_VENV := $(BASE_DIR)/libcsp/.venv

# LLVM/Clang prefix for building the analyzer (set by docker or local)
CLANG_PREFIX ?= /usr/lib/clang/22

# Board targets — native_sim wherever possible for build simplicity
ZSWATCH_BOARD := native_sim/native/64
ZMK_BOARD := nrf5340dk/nrf5340/cpuapp
ZMK_SHIELD := zmk_uno
QUADCOPTER_BOARD := native_sim
USP_ZEPHYR_BOARD := nucleo_l476rg/stm32l476xx
USP_ZEPHYR_SHIELD := semtech_sx1261mb2bas
USP_ZEPHYR_SAMPLE := source/samples/usp/lbm/periodical_uplink
CANNECTIVITY_BOARD := lpcxpresso55s16/lpc55s16
ECFW_BOARD := mec172x_mtl_s
SPINNER_BOARD := nucleo_g431rb
SPINNER_SHIELD := ihm07m1
SPINNER_BOARD_EXT := $(BASE_DIR)/spinner/spinner/boards/extensions/nucleo_g431rb/nucleo_g431rb.overlay
OPENDECK_TARGET := nrf52840dk
GRBL_BOARD := nucleo_f446re
GOLIOTH_BOARD := nrf52840dk/nrf52840
LIBRESOLAR_BOARD := mppt_1210_hus
SOF_BOARD := intel_adsp/cavs25
MICROROS_BOARD := disco_l475_iot1
FINDMYCAT_BOARD := nrf9160dk/nrf9160/ns
LINARO_BOARD := nucleo_g474re
AKIRAOS_BOARD := nrf54l15dk/nrf54l15/cpuapp
ASSETTRACKER_BOARD := native_sim
THINGSET_BOARD := xiao_esp32c3
BACNET_BOARD := nucleo_f429zi
LIBCSP_BOARD := native_sim

# Pinned commit hashes for reproducibility
QUADCOPTER_COMMIT := 3ccd3b786011579c17d270f29189d0ad574256e8
ZSWATCH_COMMIT := d06e08f56172bf87585b246fc1b84d8c441f9ece
ZMK_COMMIT := ff09f2d0c9f13a868c8f71d71d9348ade438e4b6
USP_ZEPHYR_COMMIT := bfacd435f53935ebea1a0e95fb877f4ede119985
CANNECTIVITY_COMMIT := f229e129aa0859b79f2997d1cdc0443e10d97eee
ECFW_COMMIT := 3920b48a221edf37439bd11e619e4465681ddf6c
OPENDECK_COMMIT := a45237ab1d8654fb4425fabbbd8664272962c58e
SPINNER_COMMIT := 06be1520d592f4cbaa32ee93d07283d0824f6a55
GRBL_COMMIT := 00b6416cf1aecb5c833278f5d9a1e0a77b8898ae
LIBRESOLAR_COMMIT := 575ae8ba46c977beab54b6106195a53776c9633a
GOLIOTH_COMMIT := f15281671a9dd5c14a5215021f4469a8e3071b37
SOF_COMMIT := 56c9cb8e936302955ad05412b7bc266cf4b74078
MICROROS_COMMIT := 229ffc0e131ee7942db3bb2e731fb8851583bb25
FINDMYCAT_COMMIT := c18e31b185168eb25c2c84108b198cd8c160b9d5
LINARO_COMMIT := 8d9207f6896f7553803ac4a6c340320f8459d284
AKIRAOS_COMMIT := 761e9b26cb904480be7186be10840a618cc694c0
ASSETTRACKER_COMMIT := 9c8e6ab1adb31c651b493e58dda4d00191432fcb
THINGSET_COMMIT := 8b32f7db1898dc51daa1834b3f8058de6a67166f
BACNET_COMMIT := 52cd4aac1a3502816108c71c6e480b185abed412
LIBCSP_COMMIT := a47b4d7302360a01993c83bdb0de514240002fc4

# Pinned Zephyr revisions (matched to west.yml or overridden for compatibility)
QUADCOPTER_ZEPHYR_REVISION := v4.4.0

# Per-project ZEPHYR_BASE (inside each project's workspace)
ZSWATCH_ZEPHYR := $(BASE_DIR)/zswatch/source/zephyr
ZMK_ZEPHYR := $(BASE_DIR)/zmk/source/zephyr
QUADCOPTER_ZEPHYR := $(BASE_DIR)/quadcopter/source/zephyr
USP_ZEPHYR_ZEPHYR := $(BASE_DIR)/usp-zephyr/zephyr
CANNECTIVITY_ZEPHYR := $(BASE_DIR)/cannectivity/zephyr
ECFW_ZEPHYR := $(BASE_DIR)/ecfw/ecfwwork/zephyr_fork
OPENDECK_ZEPHYR := $(BASE_DIR)/opendeck/zephyr
SPINNER_ZEPHYR := $(BASE_DIR)/spinner/zephyr
GRBL_ZEPHYR := $(BASE_DIR)/grbl/zephyr
GOLIOTH_ZEPHYR := $(BASE_DIR)/golioth/zephyr
LIBRESOLAR_ZEPHYR := $(BASE_DIR)/libresolar/zephyr
SOF_ZEPHYR := $(BASE_DIR)/sof/zephyr
MICROROS_ZEPHYR := $(BASE_DIR)/microros/zephyr
FINDMYCAT_ZEPHYR := $(BASE_DIR)/findmycat/source/packages/outdoor-location-engine/zephyr
LINARO_ZEPHYR := $(BASE_DIR)/linaro/zephyr
AKIRAOS_ZEPHYR := $(BASE_DIR)/akiraos/zephyr
ASSETTRACKER_ZEPHYR := $(BASE_DIR)/assettracker/zephyr
THINGSET_ZEPHYR := $(BASE_DIR)/thingset/zephyr
BACNET_ZEPHYR := $(BASE_DIR)/bacnet/zephyr
LIBCSP_ZEPHYR := $(BASE_DIR)/libcsp/zephyr

# Analysis output
ANALYSIS_DIR := $(CURDIR)/test-results

# Analyzer flags
ANALYZER_FLAGS := --output=json --impact-all --scope=all --filter-kernel-symbols --extra-arg=-isystem --extra-arg=$(CLANG_PREFIX)/include

# B min-cut / restart-partition flags. The restart_set partition is always
# emitted in the JSON output (in addition to affected_threads); these flags
# control how the partition splits mandatory vs advisory:
#   --impact-hop-depth=1          : mandatory = SCC + first-order critical/high
#                                   blocking edges; deeper hops become advisory
#   --impact-advisory-severity=MEDIUM : edges at or below MEDIUM are advisory
#   --impact-omnipresent=false    : exclude highlyConnected (omnipresent)
#                                   subsystem primitives from propagation graph
#   --allow-leaks                 : leak-tolerant heap recovery — [system_heap]
#                                   edges downgraded to LOW and excluded from
#                                   propagation (heap users no longer restart
#                                   together; reported in filter_skipped_objects)
# Override per-invocation, e.g. `make analyze-spinner MINCUT_FLAGS=` to revert
# to the historical full-transitive-only behaviour.
MINCUT_FLAGS := --impact-hop-depth=1 --impact-advisory-severity=MEDIUM --impact-omnipresent=true --allow-leaks

# Parallel job count for analyze-all / analyze. Each analyzer instance
# uses significant CPU (Clang parsing); default is nproc for maximum
# parallelism. Override with `make analyze-all JOBS=4` or `JOBS=1` for
# sequential execution.
JOBS ?= $(shell nproc)

# Extra analyzer flags for ARM-targeted projects (non-native_sim)
ARM_TARGET_FLAGS := --extra-arg=--target=arm-none-eabi
RISCV_TARGET_FLAGS := --extra-arg=--target=riscv64-unknown-elf

# Per-project app-source path prefixes. The analyzer classifies a thread/object
# as user vs system by whether its source file lives under this prefix. Paths
# must be absolute (matching the absolute path stored in compile_commands.json
# after Clang resolves them). /work/ is the Docker mount point used by make.sh.
ZSWATCH_APP_PREFIX       := /work/zswatch/source/app
ZMK_APP_PREFIX           := /work/zmk/source/app
QUADCOPTER_APP_PREFIX    := /work/quadcopter/source/controller/app
USP_ZEPHYR_APP_PREFIX    := /work/usp-zephyr/source
CANNECTIVITY_APP_PREFIX  := /work/cannectivity/cannectivity/app
ECFW_APP_PREFIX          := /work/ecfw/ecfwwork
OPENDECK_APP_PREFIX      := /work/opendeck/OpenDeck/app
SPINNER_APP_PREFIX       := /work/spinner/spinner
GRBL_APP_PREFIX          := /work/grbl/source/src
GOLIOTH_APP_PREFIX       := /work/golioth/modules
LIBRESOLAR_APP_PREFIX    := /work/libresolar/charge-controller-firmware/app
SOF_APP_PREFIX           := /work/sof/sof
MICROROS_APP_PREFIX      := /work/microros/microros-app
FINDMYCAT_APP_PREFIX     := /work/findmycat/source/packages/outdoor-location-engine/app
LINARO_APP_PREFIX        := /work/linaro/source
AKIRAOS_APP_PREFIX       := /work/akiraos/source
ASSETTRACKER_APP_PREFIX  := /work/assettracker/source/app
THINGSET_APP_PREFIX      := /work/thingset/source
BACNET_APP_PREFIX        := /work/bacnet/source
LIBCSP_APP_PREFIX        := /work/libcsp/source/src

.PHONY: all clone build analyze analyze-all clean distclean build-analyzer clean-analyzer \
        clone-zswatch build-zswatch analyze-zswatch clean-zswatch \
        clone-zmk build-zmk analyze-zmk clean-zmk \
        clone-quadcopter build-quadcopter analyze-quadcopter clean-quadcopter \
        clone-usp-zephyr build-usp-zephyr analyze-usp-zephyr clean-usp-zephyr \
        clone-cannectivity build-cannectivity analyze-cannectivity clean-cannectivity \
        clone-ecfw build-ecfw analyze-ecfw clean-ecfw \
        clone-opendeck build-opendeck analyze-opendeck clean-opendeck \
        clone-spinner build-spinner analyze-spinner clean-spinner \
        clone-grbl build-grbl analyze-grbl clean-grbl \
        clone-golioth build-golioth analyze-golioth clean-golioth \
        clone-libresolar build-libresolar analyze-libresolar clean-libresolar \
        clone-sof build-sof analyze-sof clean-sof \
        clone-microros build-microros analyze-microros clean-microros \
         clone-findmycat build-findmycat analyze-findmycat clean-findmycat \
         clone-linaro build-linaro analyze-linaro clean-linaro \
         clone-akiraos build-akiraos analyze-akiraos clean-akiraos \
         clone-assettracker build-assettracker analyze-assettracker clean-assettracker \
         clone-thingset build-thingset analyze-thingset clean-thingset \
         clone-bacnet build-bacnet analyze-bacnet clean-bacnet \
         clone-libcsp build-libcsp analyze-libcsp clean-libcsp

all: clone build analyze

build-analyzer:
	cd $(CURDIR)/analyzer && cmake -B build -S . \
		-DCMAKE_BUILD_TYPE=Release \
		-DLLVM_DIR=$$(llvm-config-22 --cmakedir) \
		-DClang_DIR=$$(llvm-config-22 --cmakedir)/../clang \
		-DCMAKE_C_COMPILER=clang-22 \
		-DCMAKE_CXX_COMPILER=clang++-22
	$(MAKE) -C $(CURDIR)/analyzer/build

# ============================================================
# ZSWatch
# ============================================================
$(BASE_DIR)/zswatch/.cloned:
	@echo "=== Cloning ZSWatch ==="
	mkdir -p $(BASE_DIR)/zswatch
	python3 -m venv $(ZSWATCH_VENV)
	$(ZSWATCH_VENV)/bin/pip install --upgrade pip west
	git clone https://github.com/ZSWatch/ZSWatch.git $(BASE_DIR)/zswatch/source
	cd $(BASE_DIR)/zswatch/source && git checkout $(ZSWATCH_COMMIT)
	cd $(BASE_DIR)/zswatch/source && unset ZEPHYR_BASE && PATH="$(ZSWATCH_VENV)/bin:$$PATH" west init -l app
	cd $(BASE_DIR)/zswatch/source && unset ZEPHYR_BASE && PATH="$(ZSWATCH_VENV)/bin:$$PATH" west update
	@echo "=== Patching ZSWatch for SDK compatibility ==="
	cd $(BASE_DIR)/zswatch/source && patch -p1 < $(CURDIR)/src/zswatch/01-fix-sdk-version.patch
	cd $(BASE_DIR)/zswatch/source && PATH="$(ZSWATCH_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	cd $(BASE_DIR)/zswatch/source && PATH="$(ZSWATCH_VENV)/bin:$$PATH" pip install -q -r app/scripts/requirements.txt || true
	@touch $@

clone-zswatch: $(BASE_DIR)/zswatch/.cloned

build-zswatch: $(BASE_DIR)/zswatch/.cloned
	@echo "=== Building ZSWatch for $(ZSWATCH_BOARD) ==="
	cd $(BASE_DIR)/zswatch/source && \
		ZEPHYR_BASE=$(ZSWATCH_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(ZSWATCH_VENV)/bin:$$PATH" \
		west build --pristine --build-dir app/build app \
		--board $(ZSWATCH_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DSB_CONF_FILE=sysbuild_no_mcuboot_no_xip.conf

analyze-zswatch: build-zswatch build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing ZSWatch ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/zswatch/source/app/build/app/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/zswatch-analysis.json \
		--app-source-prefix=$(ZSWATCH_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS)

clean-zswatch:
	rm -rf $(BASE_DIR)/zswatch/source/app/build

# ============================================================
# ZMK Firmware
# ============================================================
$(BASE_DIR)/zmk/.cloned:
	@echo "=== Cloning ZMK ==="
	mkdir -p $(BASE_DIR)/zmk
	python3 -m venv $(ZMK_VENV)
	$(ZMK_VENV)/bin/pip install --upgrade pip west
	git clone https://github.com/zmkfirmware/zmk.git $(BASE_DIR)/zmk/source
	cd $(BASE_DIR)/zmk/source && git checkout $(ZMK_COMMIT)
	cd $(BASE_DIR)/zmk/source && unset ZEPHYR_BASE && PATH="$(ZMK_VENV)/bin:$$PATH" west init -l app
	cd $(BASE_DIR)/zmk/source && unset ZEPHYR_BASE && PATH="$(ZMK_VENV)/bin:$$PATH" west update
	@echo "=== Patching ZMK for SDK compatibility ==="
	cd $(BASE_DIR)/zmk/source && patch -p1 < $(CURDIR)/src/zmk/01-fix-sdk-version.patch
	cd $(BASE_DIR)/zmk/source && PATH="$(ZMK_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@touch $@

clone-zmk: $(BASE_DIR)/zmk/.cloned

build-zmk: $(BASE_DIR)/zmk/.cloned
	@echo "=== Building ZMK for $(ZMK_BOARD) with shield $(ZMK_SHIELD) ==="
	cd $(BASE_DIR)/zmk/source && \
		ZEPHYR_BASE=$(ZMK_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(ZMK_VENV)/bin:$$PATH" \
		west build --pristine --build-dir app/build app \
		--board $(ZMK_BOARD) --shield $(ZMK_SHIELD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DCONFIG_MINIMAL_LIBC=y \
		-DCONFIG_ZMK_USB=y \
		-DCONFIG_ZMK_BLE=y

analyze-zmk: build-zmk build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing ZMK ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/zmk/source/app/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/zmk-analysis.json \
		--app-source-prefix=$(ZMK_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-zmk:
	rm -rf $(BASE_DIR)/zmk/source/app/build

# ============================================================
# Quadcopter
# ============================================================
$(BASE_DIR)/quadcopter/.cloned:
	@echo "=== Cloning Quadcopter ==="
	mkdir -p $(BASE_DIR)/quadcopter
	python3 -m venv $(QUADCOPTER_VENV)
	$(QUADCOPTER_VENV)/bin/pip install --upgrade pip west
	git clone https://github.com/MaroMetelski/quadcopter.git $(BASE_DIR)/quadcopter/source
	cd $(BASE_DIR)/quadcopter/source && git checkout $(QUADCOPTER_COMMIT)
	@echo "=== Patching Quadcopter for native_sim ==="
	cd $(BASE_DIR)/quadcopter/source && patch -p1 < $(CURDIR)/src/quadcopter/01-fix-build-for-v4.4.patch
	cp $(CURDIR)/src/quadcopter/native_sim.conf $(BASE_DIR)/quadcopter/source/controller/app/boards/native_sim.conf
	cp $(CURDIR)/src/quadcopter/native_stubs.c $(BASE_DIR)/quadcopter/source/controller/app/src/platform/native_stubs.c
	@echo "=== Running west init/update ==="
	cd $(BASE_DIR)/quadcopter/source && unset ZEPHYR_BASE && PATH="$(QUADCOPTER_VENV)/bin:$$PATH" west init -l controller
	cd $(BASE_DIR)/quadcopter/source && unset ZEPHYR_BASE && PATH="$(QUADCOPTER_VENV)/bin:$$PATH" west update
	cd $(BASE_DIR)/quadcopter/source && PATH="$(QUADCOPTER_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@touch $@

clone-quadcopter: $(BASE_DIR)/quadcopter/.cloned

build-quadcopter: $(BASE_DIR)/quadcopter/.cloned
	@echo "=== Building Quadcopter for $(QUADCOPTER_BOARD) ==="
	cd $(BASE_DIR)/quadcopter/source && \
		ZEPHYR_BASE=$(QUADCOPTER_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(QUADCOPTER_VENV)/bin:$$PATH" \
		west build --pristine --build-dir controller/build controller/app \
		--board $(QUADCOPTER_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

analyze-quadcopter: build-quadcopter build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing Quadcopter ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/quadcopter/source/controller/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/quadcopter-analysis.json \
		--app-source-prefix=$(QUADCOPTER_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS)

clean-quadcopter:
	rm -rf $(BASE_DIR)/quadcopter/source/controller/build

# ============================================================
# LoRa USP Zephyr
# ============================================================
$(BASE_DIR)/usp-zephyr/.cloned:
	@echo "=== Cloning LoRa USP Zephyr ==="
	mkdir -p $(BASE_DIR)/usp-zephyr
	python3 -m venv $(USP_ZEPHYR_VENV)
	$(USP_ZEPHYR_VENV)/bin/pip install --upgrade pip west
	git clone https://github.com/Lora-net/usp_zephyr.git $(BASE_DIR)/usp-zephyr/source
	cd $(BASE_DIR)/usp-zephyr/source && git checkout $(USP_ZEPHYR_COMMIT)
	cd $(BASE_DIR)/usp-zephyr && unset ZEPHYR_BASE && PATH="$(USP_ZEPHYR_VENV)/bin:$$PATH" west init -l source
	cd $(BASE_DIR)/usp-zephyr && unset ZEPHYR_BASE && PATH="$(USP_ZEPHYR_VENV)/bin:$$PATH" west update
	@echo "=== Patching USP Zephyr for SDK compatibility ==="
	cd $(BASE_DIR)/usp-zephyr/zephyr && patch -p1 < $(CURDIR)/src/usp-zephyr/01-fix-sdk-version.patch
	@echo "=== Patching USP Zephyr dev_env.cmake (lr_fhss path fix) ==="
	cd $(BASE_DIR)/usp-zephyr/source && patch -p1 < $(CURDIR)/src/usp-zephyr/02-fix-dev-env-lr-fhss-path.patch
	cd $(BASE_DIR)/usp-zephyr && PATH="$(USP_ZEPHYR_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@touch $@

clone-usp-zephyr: $(BASE_DIR)/usp-zephyr/.cloned

build-usp-zephyr: $(BASE_DIR)/usp-zephyr/.cloned
	@echo "=== Building LoRa USP for $(USP_ZEPHYR_BOARD) ==="
	cd $(BASE_DIR)/usp-zephyr && \
		ZEPHYR_BASE=$(USP_ZEPHYR_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(USP_ZEPHYR_VENV)/bin:$$PATH" \
		west build --pristine \
		--board $(USP_ZEPHYR_BOARD) \
		--shield $(USP_ZEPHYR_SHIELD) \
		$(USP_ZEPHYR_SAMPLE) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

analyze-usp-zephyr: build-usp-zephyr build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing LoRa USP ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/usp-zephyr/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/usp-zephyr-analysis.json \
		--app-source-prefix=$(USP_ZEPHYR_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-usp-zephyr:
	rm -rf $(BASE_DIR)/usp-zephyr/build

# ============================================================
# CANnectivity
# ============================================================
$(BASE_DIR)/cannectivity/.cloned:
	@echo "=== Cloning CANnectivity ==="
	mkdir -p $(BASE_DIR)/cannectivity
	python3 -m venv $(CANNECTIVITY_VENV)
	$(CANNECTIVITY_VENV)/bin/pip install --upgrade pip west
	cd $(BASE_DIR)/cannectivity && unset ZEPHYR_BASE && PATH="$(CANNECTIVITY_VENV)/bin:$$PATH" west init -m https://github.com/CANnectivity/cannectivity --mr v1.4.0
	cd $(BASE_DIR)/cannectivity/cannectivity && git checkout $(CANNECTIVITY_COMMIT)
	@echo "=== Patching CANnectivity: pin Zephyr to v4.4.0 ==="
	cd $(BASE_DIR)/cannectivity/cannectivity && patch -p1 < $(CURDIR)/src/cannectivity/01-pin-zephyr-revision.patch
	cd $(BASE_DIR)/cannectivity && unset ZEPHYR_BASE && PATH="$(CANNECTIVITY_VENV)/bin:$$PATH" west update
	cd $(BASE_DIR)/cannectivity && PATH="$(CANNECTIVITY_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@touch $@

clone-cannectivity: $(BASE_DIR)/cannectivity/.cloned

build-cannectivity: $(BASE_DIR)/cannectivity/.cloned
	@echo "=== Building CANnectivity for $(CANNECTIVITY_BOARD) ==="
	cd $(BASE_DIR)/cannectivity && \
		ZEPHYR_BASE=$(CANNECTIVITY_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(CANNECTIVITY_VENV)/bin:$$PATH" \
		west build --pristine=always cannectivity/app \
		--board $(CANNECTIVITY_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

analyze-cannectivity: build-cannectivity build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing CANnectivity ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/cannectivity/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/cannectivity-analysis.json \
		--app-source-prefix=$(CANNECTIVITY_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-cannectivity:
	rm -rf $(BASE_DIR)/cannectivity/build

# ============================================================
# Intel ECFW
# ============================================================
$(BASE_DIR)/ecfw/.cloned:
	@echo "=== Cloning Intel ECFW ==="
	mkdir -p $(BASE_DIR)/ecfw
	python3 -m venv $(ECFW_VENV)
	$(ECFW_VENV)/bin/pip install --upgrade pip west
	git clone https://github.com/intel/ecfw-zephyr.git $(BASE_DIR)/ecfw/ecfwwork
	cd $(BASE_DIR)/ecfw/ecfwwork && git checkout $(ECFW_COMMIT)
	cd $(BASE_DIR)/ecfw && unset ZEPHYR_BASE && PATH="$(ECFW_VENV)/bin:$$PATH" west init -l ecfwwork
	cd $(BASE_DIR)/ecfw && unset ZEPHYR_BASE && PATH="$(ECFW_VENV)/bin:$$PATH" west update
	@echo "=== Applying ECFW patches to Zephyr v3.6 ==="
	cd $(BASE_DIR)/ecfw/ecfwwork/zephyr_fork && git config user.email "ci@example.com" && git config user.name "CI"
	cd $(BASE_DIR)/ecfw/ecfwwork/zephyr_fork && git am $(CURDIR)/src/ecfw/patches_v3_6.patch
	@echo "=== Patching ECFW Zephyr for SDK compatibility ==="
	cd $(BASE_DIR)/ecfw/ecfwwork/zephyr_fork && patch -p1 < $(CURDIR)/src/ecfw/01-fix-sdk-version.patch
	cd $(BASE_DIR)/ecfw/ecfwwork/zephyr_fork && patch -p1 < $(CURDIR)/src/ecfw/02-fix-toolchain-layout.patch
	cd $(BASE_DIR)/ecfw && PATH="$(ECFW_VENV)/bin:$$PATH" pip install -q -r ecfwwork/zephyr_fork/scripts/requirements.txt
	@touch $@

clone-ecfw: $(BASE_DIR)/ecfw/.cloned

build-ecfw: $(BASE_DIR)/ecfw/.cloned
	@echo "=== Building Intel ECFW for $(ECFW_BOARD) ==="
	cd $(BASE_DIR)/ecfw/ecfwwork && \
		ZEPHYR_BASE=$(ECFW_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(ECFW_VENV)/bin:$$PATH" \
		west build -p always -b $(ECFW_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DOVERLAY_CONFIG=debug.conf \
		-DCONFIG_MINIMAL_LIBC=y

analyze-ecfw: build-ecfw build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing Intel ECFW ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/ecfw/ecfwwork/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/ecfw-analysis.json \
		--app-source-prefix=$(ECFW_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-ecfw:
	rm -rf $(BASE_DIR)/ecfw/ecfwwork/build

# ============================================================
# OpenDeck
# ============================================================
$(BASE_DIR)/opendeck/.cloned:
	@echo "=== Cloning OpenDeck ==="
	mkdir -p $(BASE_DIR)/opendeck
	python3 -m venv $(OPENDECK_VENV)
	$(OPENDECK_VENV)/bin/pip install --upgrade pip west
	if [ ! -f $(BASE_DIR)/opendeck/.west/config ]; then \
		rm -rf $(BASE_DIR)/opendeck/.west; \
		if [ -d $(BASE_DIR)/opendeck/OpenDeck/.git ]; then \
			cd $(BASE_DIR)/opendeck && unset ZEPHYR_BASE && PATH="$(OPENDECK_VENV)/bin:$$PATH" west init -l OpenDeck; \
		else \
			cd $(BASE_DIR)/opendeck && unset ZEPHYR_BASE && PATH="$(OPENDECK_VENV)/bin:$$PATH" west init -m https://github.com/shanteacontrols/OpenDeck --mr v8.5.1; \
		fi; \
	fi
	cd $(BASE_DIR)/opendeck/OpenDeck && git checkout $(OPENDECK_COMMIT)
	cd $(BASE_DIR)/opendeck && unset ZEPHYR_BASE && PATH="$(OPENDECK_VENV)/bin:$$PATH" west update
	cd $(BASE_DIR)/opendeck && PATH="$(OPENDECK_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@echo "=== Installing dasel binary (v1 for zenv compatibility) ==="
	wget -q https://github.com/TomWright/dasel/releases/download/v1.27.1/dasel_linux_amd64 -O $(BASE_DIR)/opendeck/dasel && chmod +x $(BASE_DIR)/opendeck/dasel
	@echo "=== Installing extra Python dependencies (pyelftools, cryptography, cbor2, intelhex) ==="
	$(OPENDECK_VENV)/bin/pip install -q pyelftools cryptography cbor2 intelhex
	@echo "=== Cloning zenv build system ==="
	if [ ! -d $(BASE_DIR)/opendeck/zenv ]; then \
		git clone --depth 1 https://github.com/paradajz/zenv.git $(BASE_DIR)/opendeck/zenv; \
	fi
	@touch $@

clone-opendeck: $(BASE_DIR)/opendeck/.cloned

build-opendeck: $(BASE_DIR)/opendeck/.cloned
	@echo "=== Building OpenDeck for $(OPENDECK_TARGET) ==="
	cd $(BASE_DIR)/opendeck/OpenDeck && \
		ZEPHYR_WS=$(BASE_DIR)/opendeck \
		ZEPHYR_BASE=$(OPENDECK_ZEPHYR) \
		ZENV_PROJECT_ROOT=$(BASE_DIR)/opendeck/OpenDeck \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		TARGET=$(OPENDECK_TARGET) \
		PATH="$(OPENDECK_VENV)/bin:$(BASE_DIR)/opendeck:$$PATH" \
		bash -c '\
			set -e; \
			export TARGET=$(OPENDECK_TARGET); \
			export ZEPHYR_BASE=$(OPENDECK_ZEPHYR); \
			export ZENV_PROJECT_ROOT=$(BASE_DIR)/opendeck/OpenDeck; \
			export ZEPHYR_WS=$(BASE_DIR)/opendeck; \
			$(BASE_DIR)/opendeck/zenv/scripts/patch.sh; \
			BOARD=$$(sed -n "s/.*zephyr-board[[:space:]]*=[[:space:]]*\"\([^\"]*\)\".*/\1/p" \
				$(BASE_DIR)/opendeck/OpenDeck/app/boards/opendeck/$(OPENDECK_TARGET)/firmware.overlay | head -n1); \
			west build --build-dir $(BASE_DIR)/opendeck/OpenDeck/build/app/default/release/$(OPENDECK_TARGET) \
				-s $(BASE_DIR)/opendeck/OpenDeck/app \
				--cmake-only \
				-b $${BOARD} \
				-- \
				-DZENV_PRESET_NAME=default \
				-DZENV_BUILD_TYPE=release \
				-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
				"-DCONF_FILE=$(BASE_DIR)/opendeck/OpenDeck/app/common.conf;$(BASE_DIR)/opendeck/OpenDeck/app/release.conf;$(BASE_DIR)/opendeck/OpenDeck/app/boards/zephyr/nrf52840dk_nrf52840/firmware.conf;$(BASE_DIR)/opendeck/OpenDeck/app/firmware/common.conf;$(BASE_DIR)/opendeck/OpenDeck/app/firmware/release.conf;$(BASE_DIR)/opendeck/OpenDeck/app/common/usb.conf;$(BASE_DIR)/opendeck/OpenDeck/app/firmware/usb.conf;$(BASE_DIR)/opendeck/OpenDeck/app/firmware/ble.conf" \
				"-DDTC_OVERLAY_FILE=$(BASE_DIR)/opendeck/OpenDeck/app/boards/zephyr/nrf52840dk_nrf52840/common.overlay;$(BASE_DIR)/opendeck/OpenDeck/app/boards/zephyr/nrf52840dk_nrf52840/firmware.overlay;$(BASE_DIR)/opendeck/OpenDeck/app/boards/opendeck/nrf52840dk/common.overlay;$(BASE_DIR)/opendeck/OpenDeck/app/boards/opendeck/nrf52840dk/firmware.overlay"; \
			west build --build-dir $(BASE_DIR)/opendeck/OpenDeck/build/app/default/release/$(OPENDECK_TARGET) || true'

analyze-opendeck: build-opendeck build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing OpenDeck ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/opendeck/OpenDeck/build/app/default/release/$(OPENDECK_TARGET)/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/opendeck-analysis.json \
		--app-source-prefix=$(OPENDECK_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-opendeck:
	rm -rf $(BASE_DIR)/opendeck/build

# ============================================================
# Spinner
# ============================================================
$(BASE_DIR)/spinner/.cloned:
	@echo "=== Cloning Spinner ==="
	mkdir -p $(BASE_DIR)/spinner
	python3 -m venv $(SPINNER_VENV)
	$(SPINNER_VENV)/bin/pip install --upgrade pip west
	cd $(BASE_DIR)/spinner && unset ZEPHYR_BASE && PATH="$(SPINNER_VENV)/bin:$$PATH" west init -m https://github.com/teslabs/spinner --mr main
	cd $(BASE_DIR)/spinner/spinner && git checkout $(SPINNER_COMMIT)
	@echo "=== Patching Spinner: pin Zephyr to v4.4.0 ==="
	cd $(BASE_DIR)/spinner/spinner && patch -p1 < $(CURDIR)/src/spinner/01-pin-zephyr-revision.patch
	cd $(BASE_DIR)/spinner && unset ZEPHYR_BASE && PATH="$(SPINNER_VENV)/bin:$$PATH" west update
	cd $(BASE_DIR)/spinner && PATH="$(SPINNER_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@touch $@

clone-spinner: $(BASE_DIR)/spinner/.cloned

build-spinner: $(BASE_DIR)/spinner/.cloned
	@echo "=== Building Spinner for $(SPINNER_BOARD) with shield $(SPINNER_SHIELD) ==="
	cd $(BASE_DIR)/spinner && \
		ZEPHYR_BASE=$(SPINNER_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(SPINNER_VENV)/bin:$$PATH" \
		west build --pristine=always spinner/spinner \
		--board $(SPINNER_BOARD) -- \
		-DBOARD_ROOT=$(BASE_DIR)/spinner/spinner \
		-DDTS_ROOT=$(BASE_DIR)/spinner/spinner \
		-DDTC_OVERLAY_FILE="$(SPINNER_BOARD_EXT);$(BASE_DIR)/spinner/spinner/boards/shields/ihm07m1/ihm07m1.overlay;$(BASE_DIR)/spinner/spinner/boards/shields/ihm07m1/boards/nucleo_g431rb.overlay" \
		-DOVERLAY_CONFIG=$(BASE_DIR)/spinner/spinner/spinner/shell.conf \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

analyze-spinner: build-spinner build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing Spinner ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/spinner/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/spinner-analysis.json \
		--app-source-prefix=$(SPINNER_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-spinner:
	rm -rf $(BASE_DIR)/spinner/build

# ============================================================
# grbl-zephyr (CNC motion control)
# ============================================================
# Standalone CMake project (no west.yml). Uses Zephyr v3.5.0 as a
# west workspace manifest, clones grbl into source/ as a subdirectory.
# Submodules use SSH URLs — rewritten to HTTPS during clone.
# Targets nucleo_f446re (standard STM32F4 board) with custom overlay.

$(BASE_DIR)/grbl/.cloned:
	@echo "=== Cloning grbl-zephyr ==="
	mkdir -p $(BASE_DIR)/grbl
	python3 -m venv $(GRBL_VENV)
	$(GRBL_VENV)/bin/pip install --upgrade pip west
	cd $(BASE_DIR)/grbl && unset ZEPHYR_BASE && PATH="$(GRBL_VENV)/bin:$$PATH" west init -m https://github.com/zephyrproject-rtos/zephyr --mr v3.5.0
	cd $(BASE_DIR)/grbl && unset ZEPHYR_BASE && PATH="$(GRBL_VENV)/bin:$$PATH" west update
	@echo "=== Patching Zephyr v3.5.0 for SDK compatibility ==="
	cd $(BASE_DIR)/grbl/zephyr && patch -p1 < $(CURDIR)/src/grbl/01-fix-sdk-version.patch
	cd $(BASE_DIR)/grbl/zephyr && patch -p1 < $(CURDIR)/src/grbl/02-fix-toolchain-layout.patch
	@echo "=== Cloning grbl-zephyr source ==="
	git clone https://github.com/iwasz/zephyr-grbl.git $(BASE_DIR)/grbl/source
	cd $(BASE_DIR)/grbl/source && git checkout $(GRBL_COMMIT)
	@echo "=== Fixing submodule SSH URLs to HTTPS ==="
	cd $(BASE_DIR)/grbl/source && git config submodule.deps/TMC2130Stepper.url https://github.com/iwasz/TMC2130Stepper.git
	cd $(BASE_DIR)/grbl/source && git config submodule.deps/etl.url https://github.com/ETLCPP/etl.git
	cd $(BASE_DIR)/grbl/source && git config submodule.deps/compile-time-regular-expressions.url https://github.com/hanickadot/compile-time-regular-expressions.git
	cd $(BASE_DIR)/grbl/source && git config submodule.deps/libstate.url https://github.com/iwasz/libstate.git
	@echo "=== Patching grbl overlay for Zephyr v3.5.0 devicetree requirements ==="
	cp $(CURDIR)/src/grbl/nucleo_f446re.overlay $(BASE_DIR)/grbl/source/boards/nucleo_f446re.overlay
	cd $(BASE_DIR)/grbl/source && sed -i '/SSD1306_SH1106_COMPATIBLE/d' prj.conf
	cd $(BASE_DIR)/grbl/source && git submodule update --init deps/TMC2130Stepper deps/etl deps/compile-time-regular-expressions deps/libstate
	cd $(BASE_DIR)/grbl && PATH="$(GRBL_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@touch $@

clone-grbl: $(BASE_DIR)/grbl/.cloned

build-grbl: $(BASE_DIR)/grbl/.cloned
	@echo "=== Building grbl-zephyr for $(GRBL_BOARD) ==="
	cd $(BASE_DIR)/grbl/source && \
		ZEPHYR_BASE=$(GRBL_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(GRBL_VENV)/bin:$$PATH" \
		west build --pristine --build-dir build . \
		--board $(GRBL_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DDTC_OVERLAY_FILE=boards/nucleo_f446re.overlay || true

analyze-grbl: build-grbl build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing grbl-zephyr ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/grbl/source/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/grbl-analysis.json \
		--app-source-prefix=$(GRBL_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-grbl:
	rm -rf $(BASE_DIR)/grbl/source/build

# ============================================================
# Golioth Firmware SDK (IoT/cloud — CoAP, DTLS, OTA)
# ============================================================
$(BASE_DIR)/golioth/.cloned:
	@echo "=== Cloning Golioth Firmware SDK v0.22.0 ==="
	mkdir -p $(BASE_DIR)/golioth
	python3 -m venv $(GOLIOTH_VENV)
	$(GOLIOTH_VENV)/bin/pip install --upgrade pip west
	cd $(BASE_DIR)/golioth && unset ZEPHYR_BASE && PATH="$(GOLIOTH_VENV)/bin:$$PATH" west init -m https://github.com/golioth/golioth-firmware-sdk.git --mr v0.22.0 --mf west-zephyr.yml
	cd $(BASE_DIR)/golioth/modules/lib/golioth-firmware-sdk && git checkout $(GOLIOTH_COMMIT)
	cd $(BASE_DIR)/golioth && unset ZEPHYR_BASE && PATH="$(GOLIOTH_VENV)/bin:$$PATH" west update
	@echo "=== Patching Zephyr v4.2.1 for SDK 1.0.1 compatibility ==="
	cd $(BASE_DIR)/golioth/zephyr && patch -p1 < $(CURDIR)/src/golioth/01-fix-sdk-version.patch
	cd $(BASE_DIR)/golioth/zephyr && patch -p1 < $(CURDIR)/src/golioth/02-fix-toolchain-layout.patch
	cd $(BASE_DIR)/golioth/modules/lib/golioth-firmware-sdk && git submodule update --init --recursive
	cd $(BASE_DIR)/golioth && PATH="$(GOLIOTH_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	cd $(BASE_DIR)/golioth && PATH="$(GOLIOTH_VENV)/bin:$$PATH" pip install -q pyelftools
	@touch $@

clone-golioth: $(BASE_DIR)/golioth/.cloned

build-golioth: $(BASE_DIR)/golioth/.cloned
	@echo "=== Building Golioth hello example for $(GOLIOTH_BOARD) ==="
	cd $(BASE_DIR)/golioth && \
		ZEPHYR_BASE=$(GOLIOTH_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(GOLIOTH_VENV)/bin:$$PATH" \
		west build --pristine --build-dir build \
		modules/lib/golioth-firmware-sdk/examples/zephyr/hello \
		--board $(GOLIOTH_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

analyze-golioth: build-golioth build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing Golioth ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/golioth/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/golioth-analysis.json \
		--app-source-prefix=$(GOLIOTH_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-golioth:
	rm -rf $(BASE_DIR)/golioth/build

# ============================================================
# Libre Solar Charge Controller (MPPT/PWM, power management)
# ============================================================
$(BASE_DIR)/libresolar/.cloned:
	@echo "=== Cloning Libre Solar charge-controller-firmware ==="
	mkdir -p $(BASE_DIR)/libresolar
	python3 -m venv $(LIBRESOLAR_VENV)
	$(LIBRESOLAR_VENV)/bin/pip install --upgrade pip west
	cd $(BASE_DIR)/libresolar && unset ZEPHYR_BASE && PATH="$(LIBRESOLAR_VENV)/bin:$$PATH" west init -m https://github.com/LibreSolar/charge-controller-firmware --mr main
	cd $(BASE_DIR)/libresolar/charge-controller-firmware && git checkout $(LIBRESOLAR_COMMIT)
	cd $(BASE_DIR)/libresolar && unset ZEPHYR_BASE && PATH="$(LIBRESOLAR_VENV)/bin:$$PATH" west update
	cd $(BASE_DIR)/libresolar && PATH="$(LIBRESOLAR_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@touch $@

clone-libresolar: $(BASE_DIR)/libresolar/.cloned

build-libresolar: $(BASE_DIR)/libresolar/.cloned
	@echo "=== Building Libre Solar for $(LIBRESOLAR_BOARD) ==="
	cd $(BASE_DIR)/libresolar && \
		ZEPHYR_BASE=$(LIBRESOLAR_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(LIBRESOLAR_VENV)/bin:$$PATH" \
		west build --pristine --build-dir build \
		charge-controller-firmware/app \
		--board $(LIBRESOLAR_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

analyze-libresolar: build-libresolar build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing Libre Solar ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/libresolar/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/libresolar-analysis.json \
		--app-source-prefix=$(LIBRESOLAR_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-libresolar:
	rm -rf $(BASE_DIR)/libresolar/build

# ============================================================
# Sound Open Firmware (SOF) — audio DSP pipeline
# ============================================================
$(BASE_DIR)/sof/.cloned:
	@echo "=== Cloning SOF (Sound Open Firmware) ==="
	mkdir -p $(BASE_DIR)/sof
	python3 -m venv $(SOF_VENV)
	$(SOF_VENV)/bin/pip install --upgrade pip west
	cd $(BASE_DIR)/sof && unset ZEPHYR_BASE && PATH="$(SOF_VENV)/bin:$$PATH" west init -m https://github.com/zephyrproject-rtos/sof --mr zephyr
	cd $(BASE_DIR)/sof/sof && git checkout $(SOF_COMMIT)
	cd $(BASE_DIR)/sof && unset ZEPHYR_BASE && PATH="$(SOF_VENV)/bin:$$PATH" west update
	@echo "=== Patching Zephyr for SDK 1.0.1 compatibility ==="
	cd $(BASE_DIR)/sof/zephyr && patch -p1 < $(CURDIR)/src/sof/01-fix-sdk-version.patch
	cd $(BASE_DIR)/sof/zephyr && patch -p1 < $(CURDIR)/src/sof/02-fix-toolchain-layout.patch
	cd $(BASE_DIR)/sof && PATH="$(SOF_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@touch $@

clone-sof: $(BASE_DIR)/sof/.cloned

build-sof: $(BASE_DIR)/sof/.cloned
	@echo "=== Building SOF for $(SOF_BOARD) ==="
	cd $(BASE_DIR)/sof && \
		ZEPHYR_BASE=$(SOF_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(SOF_VENV)/bin:$$PATH" \
		west build --pristine --build-dir build \
		sof/app \
		--board $(SOF_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

analyze-sof: build-sof build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing SOF ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/sof/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/sof-analysis.json \
		--app-source-prefix=$(SOF_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS)

clean-sof:
	rm -rf $(BASE_DIR)/sof/build

# ============================================================
# micro-ROS Zephyr module (Robotics — ROS 2, DDS, pub/sub)
# ============================================================
$(BASE_DIR)/microros/.cloned:
	@echo "=== Cloning micro-ROS Zephyr module ==="
	mkdir -p $(BASE_DIR)/microros
	python3 -m venv $(MICROROS_VENV)
	$(MICROROS_VENV)/bin/pip install --upgrade pip west
	cd $(BASE_DIR)/microros && unset ZEPHYR_BASE && PATH="$(MICROROS_VENV)/bin:$$PATH" west init -m https://github.com/zephyrproject-rtos/zephyr --mr v4.4.0
	cd $(BASE_DIR)/microros && unset ZEPHYR_BASE && PATH="$(MICROROS_VENV)/bin:$$PATH" west update
	cd $(BASE_DIR)/microros && PATH="$(MICROROS_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@echo "=== Installing colcon and ROS 2 build tools ==="
	cd $(BASE_DIR)/microros && PATH="$(MICROROS_VENV)/bin:$$PATH" pip install -q catkin_pkg lark empy colcon-common-extensions
	@echo "=== Cloning micro-ROS module repo ==="
	cd $(BASE_DIR)/microros && git clone https://github.com/micro-ROS/micro_ros_zephyr_module microros-app
	cd $(BASE_DIR)/microros/microros-app && git checkout $(MICROROS_COMMIT)
	@echo "=== Patching micro-ROS prj.conf for Zephyr v4.4.0 ==="
	cp $(CURDIR)/src/microros/prj.conf $(BASE_DIR)/microros/microros-app/prj.conf
	@touch $@

clone-microros: $(BASE_DIR)/microros/.cloned

build-microros: $(BASE_DIR)/microros/.cloned
	@echo "=== Building micro-ROS for $(MICROROS_BOARD) ==="
	cd $(BASE_DIR)/microros/microros-app && \
		ZEPHYR_BASE=$(MICROROS_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(MICROROS_VENV)/bin:$$PATH" \
		west build --pristine --build-dir build . \
		--board $(MICROROS_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON || true

analyze-microros: build-microros build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing micro-ROS ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/microros/microros-app/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/microros-analysis.json \
		--app-source-prefix=$(MICROROS_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-microros:
	rm -rf $(BASE_DIR)/microros/microros-app/build

# ============================================================
# FindMyCat Outdoor Location Engine (Pet tracker — LTE-M, GPS, MQTT-SN)
# ============================================================
# Uses nrfconnect/sdk-nrf v3.2.4 (NCS) which imports Zephyr v4.x.
# Board: nrf9160dk/nrf9160/ns (Cortex-M33 with TrustZone NS).
# App threads: main + UDP listener + system workqueue + modem/location library threads.

FINDMYCAT_WS := $(BASE_DIR)/findmycat/source/packages/outdoor-location-engine

$(BASE_DIR)/findmycat/.cloned:
	@echo "=== Cloning FindMyCat embedded-software ==="
	mkdir -p $(BASE_DIR)/findmycat
	python3 -m venv $(FINDMYCAT_VENV)
	$(FINDMYCAT_VENV)/bin/pip install --upgrade pip west
	if [ ! -d $(BASE_DIR)/findmycat/source/.git ]; then \
		git clone https://github.com/FindMyCat/embedded-software.git $(BASE_DIR)/findmycat/source; \
	fi
	cd $(BASE_DIR)/findmycat/source && git checkout $(FINDMYCAT_COMMIT)
	if [ ! -f $(FINDMYCAT_WS)/.west/config ]; then \
		cd $(FINDMYCAT_WS) && unset ZEPHYR_BASE && PATH="$(FINDMYCAT_VENV)/bin:$$PATH" west init -l manifest; \
	fi
	cd $(FINDMYCAT_WS) && unset ZEPHYR_BASE && PATH="$(FINDMYCAT_VENV)/bin:$$PATH" west update
	@echo "=== Patching Zephyr v4.2.99 for SDK 1.0.1 compatibility ==="
	cd $(FINDMYCAT_WS)/zephyr && git checkout -- .
	cd $(FINDMYCAT_WS)/zephyr && patch -p1 < $(CURDIR)/src/findmycat/01-fix-sdk-version.patch
	cd $(FINDMYCAT_WS)/zephyr && patch -p1 < $(CURDIR)/src/findmycat/02-fix-toolchain-layout.patch
	@echo "=== Fixing case sensitivity: NewUdpListener.c -> NewUDPListener.c ==="
	cd $(FINDMYCAT_WS)/app/src/UDPListener && ln -sf NewUdpListener.c NewUDPListener.c && ln -sf NewUdpListener.h NewUDPListener.h
	cd $(FINDMYCAT_WS) && PATH="$(FINDMYCAT_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@touch $@

clone-findmycat: $(BASE_DIR)/findmycat/.cloned

build-findmycat: $(BASE_DIR)/findmycat/.cloned
	@echo "=== Building FindMyCat for $(FINDMYCAT_BOARD) ==="
	cd $(FINDMYCAT_WS) && \
		ZEPHYR_BASE=$(FINDMYCAT_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(FINDMYCAT_VENV)/bin:$$PATH" \
		west build --pristine --build-dir build app \
		--board $(FINDMYCAT_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON || true

analyze-findmycat: build-findmycat build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing FindMyCat ==="
	$(ANALYZER) \
		--use-compdb $(FINDMYCAT_WS)/build/app/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/findmycat-analysis.json \
		--app-source-prefix=$(FINDMYCAT_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-findmycat:
	rm -rf $(FINDMYCAT_WS)/build

# ============================================================
# Linaro STeP (Secure Telemetry Pipeline — FOC motor controller)
# ============================================================
# Uses Linaro/step repo with foc_controller sample (nucleo_g474re board).
# Threads: main + rotor_sample_tid (K_THREAD_DEFINE) + step_pm_work_q (k_work_queue_start)
# Shared: step_pm_reg_access mutex, step_sp_alloc_mtx mutex, step_pm_work_q
# Zephyr pinned to v4.4.0 (west.yml originally tracks main).
# Note: fusion_imu sample requires ZSL (Zephyr Scientific Library) which is not
# available as a Zephyr west module in v4.4.0, so foc_controller is used instead.

$(BASE_DIR)/linaro/.cloned:
	@echo "=== Cloning Linaro STeP ==="
	mkdir -p $(BASE_DIR)/linaro
	python3 -m venv $(LINARO_VENV)
	$(LINARO_VENV)/bin/pip install --upgrade pip west
	if [ ! -d $(BASE_DIR)/linaro/source/.git ]; then \
		git clone https://github.com/Linaro/step.git $(BASE_DIR)/linaro/source; \
	fi
	cd $(BASE_DIR)/linaro/source && git checkout $(LINARO_COMMIT)
	@echo "=== Pinning Zephyr to v4.4.0 in west.yml ==="
	cd $(BASE_DIR)/linaro/source && sed -i 's/revision: main/revision: v4.4.0/' west.yml
	if [ ! -f $(BASE_DIR)/linaro/.west/config ]; then \
		cd $(BASE_DIR)/linaro/source && unset ZEPHYR_BASE && PATH="$(LINARO_VENV)/bin:$$PATH" west init -l .; \
	fi
	cd $(BASE_DIR)/linaro/source && unset ZEPHYR_BASE && PATH="$(LINARO_VENV)/bin:$$PATH" west update
	cd $(BASE_DIR)/linaro && PATH="$(LINARO_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@echo "=== Patching Linaro STeP for Zephyr v4.4.0 I2C API change ==="
	cd $(BASE_DIR)/linaro/source && git checkout -- .
	cd $(BASE_DIR)/linaro/source && patch -p1 < $(CURDIR)/src/linaro/01-fix-i2c-api.patch
	@touch $@

clone-linaro: $(BASE_DIR)/linaro/.cloned

build-linaro: $(BASE_DIR)/linaro/.cloned
	@echo "=== Building Linaro STeP foc_controller for $(LINARO_BOARD) ==="
	cd $(BASE_DIR)/linaro/source && \
		ZEPHYR_BASE=$(LINARO_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(LINARO_VENV)/bin:$$PATH" \
		west build --pristine --build-dir build samples/foc_controller \
		--board $(LINARO_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON || true

analyze-linaro: build-linaro build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing Linaro STeP ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/linaro/source/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/linaro-analysis.json \
		--app-source-prefix=$(LINARO_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-linaro:
	rm -rf $(BASE_DIR)/linaro/source/build

# ============================================================
# AkiraOS — WASM runtime OS with thread-per-app model
# Repo: ArturR0k3r/AkiraOS (commit 761e9b26)
# Zephyr v4.3.0 (pinned in west.yml), SDK 1.0.1 + version/toolchain patches
# Board: nrf54l15dk/nrf54l15/cpuapp (ARM Cortex-M33, BT, USB, 802.15.4)
# Submodules: modules/wasm-micro-runtime (WAMR fork), AkiraSDK (header-only)
# Threads: main + system workqueue + BT RX/TX + shell + USB + WASM app threads
#   (dynamic via k_thread_create) + logger + idle + ISR
# Shared: g_apps[] (g_runtime_mutex), g_registry[] (g_registry_mutex),
#   g_exit_cb, g_state_cb, g_sessions[], IPC state, g_restart_work
# ============================================================

$(BASE_DIR)/akiraos/.cloned:
	@echo "=== Cloning AkiraOS ==="
	mkdir -p $(BASE_DIR)/akiraos
	python3 -m venv $(AKIRAOS_VENV)
	$(AKIRAOS_VENV)/bin/pip install --upgrade pip west
	if [ ! -d $(BASE_DIR)/akiraos/source/.git ]; then \
		git clone --recursive https://github.com/ArturR0k3r/AkiraOS.git $(BASE_DIR)/akiraos/source; \
	fi
	cd $(BASE_DIR)/akiraos/source && git checkout $(AKIRAOS_COMMIT)
	cd $(BASE_DIR)/akiraos/source && git submodule update --init --recursive
	if [ ! -f $(BASE_DIR)/akiraos/.west/config ]; then \
		cd $(BASE_DIR)/akiraos/source && unset ZEPHYR_BASE && PATH="$(AKIRAOS_VENV)/bin:$$PATH" west init -l .; \
	fi
	cd $(BASE_DIR)/akiraos/source && unset ZEPHYR_BASE && PATH="$(AKIRAOS_VENV)/bin:$$PATH" west update
	cd $(BASE_DIR)/akiraos && PATH="$(AKIRAOS_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@echo "=== Patching Zephyr v4.3.0 for SDK 1.0.1 compatibility ==="
	cd $(BASE_DIR)/akiraos && git -C zephyr checkout -- .
	cd $(BASE_DIR)/akiraos && patch -p1 -d zephyr < $(CURDIR)/src/akiraos/01-fix-sdk-version.patch
	cd $(BASE_DIR)/akiraos && patch -p1 -d zephyr < $(CURDIR)/src/akiraos/02-fix-toolchain-layout.patch
	@touch $@

clone-akiraos: $(BASE_DIR)/akiraos/.cloned

build-akiraos: $(BASE_DIR)/akiraos/.cloned
	@echo "=== Building AkiraOS for $(AKIRAOS_BOARD) ==="
	cd $(BASE_DIR)/akiraos/source && \
		ZEPHYR_BASE=$(AKIRAOS_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(AKIRAOS_VENV)/bin:$$PATH" \
		west build --pristine --build-dir build . \
		--board $(AKIRAOS_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DMODULE_EXT_ROOT=$(BASE_DIR)/akiraos/source || true

analyze-akiraos: build-akiraos build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing AkiraOS ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/akiraos/source/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/akiraos-analysis.json \
		--app-source-prefix=$(AKIRAOS_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-akiraos:
	rm -rf $(BASE_DIR)/akiraos/source/build

# ============================================================
# Asset-Tracker-Template — Nordic cellular asset tracker with zbus
# Repo: nrfconnect/Asset-Tracker-Template (commit 9c8e6ab1)
# NCS v3.4.0-rc2 (Zephyr ncs-v3.4.0-rc2 fork, SDK 1.0 compatible)
# Board: native_sim (no SDK patches needed — NCS fork already supports SDK 1.0)
# Threads: main + module threads (network, cloud, FOTA, location, storage,
#   button, LED, environmental, power) via zbus channels + SMF state machine
# ============================================================

$(BASE_DIR)/assettracker/.cloned:
	@echo "=== Cloning Asset-Tracker-Template ==="
	mkdir -p $(BASE_DIR)/assettracker
	python3 -m venv $(ASSETTRACKER_VENV)
	$(ASSETTRACKER_VENV)/bin/pip install --upgrade pip west
	if [ ! -d $(BASE_DIR)/assettracker/source/.git ]; then \
		git clone https://github.com/nrfconnect/Asset-Tracker-Template.git $(BASE_DIR)/assettracker/source; \
	fi
	cd $(BASE_DIR)/assettracker/source && git checkout $(ASSETTRACKER_COMMIT)
	if [ ! -f $(BASE_DIR)/assettracker/.west/config ]; then \
		cd $(BASE_DIR)/assettracker/source && unset ZEPHYR_BASE && PATH="$(ASSETTRACKER_VENV)/bin:$$PATH" west init -l .; \
	fi
	cd $(BASE_DIR)/assettracker/source && unset ZEPHYR_BASE && PATH="$(ASSETTRACKER_VENV)/bin:$$PATH" west update
	cd $(BASE_DIR)/assettracker && PATH="$(ASSETTRACKER_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@echo "=== Installing zcbor for CBOR code generation ==="
	$(ASSETTRACKER_VENV)/bin/pip install -q "zcbor" "cbor2<5.5"
	@echo "=== Disabling MCUboot/TFM for native_sim build ==="
	cd $(BASE_DIR)/assettracker/source && sed -i \
		-e 's/^CONFIG_TFM_PROFILE_TYPE_MINIMAL=y/# CONFIG_TFM_PROFILE_TYPE_MINIMAL=y/' \
		-e 's/^CONFIG_TFM_PSA_FRAMEWORK_HAS_MM_IOVEC=y/# CONFIG_TFM_PSA_FRAMEWORK_HAS_MM_IOVEC=y/' \
		-e 's/^CONFIG_TFM_LOG_LEVEL_SILENCE=n/# CONFIG_TFM_LOG_LEVEL_SILENCE=n/' \
		-e 's/^CONFIG_TFM_SECURE_UART0=y/# CONFIG_TFM_SECURE_UART0=y/' \
		-e 's/^CONFIG_TFM_SECURE_UART_SHARE_INSTANCE=y/# CONFIG_TFM_SECURE_UART_SHARE_INSTANCE=y/' \
		-e 's/^CONFIG_TFM_EXCEPTION_INFO_DUMP=y/# CONFIG_TFM_EXCEPTION_INFO_DUMP=y/' \
		-e 's/^CONFIG_TFM_SPM_LOG_LEVEL_DEBUG=y/# CONFIG_TFM_SPM_LOG_LEVEL_DEBUG=y/' \
		-e 's/^CONFIG_NRF_CLOUD_SEND_DEVICE_INFO_BOOTLOADER_VERSION=y/# CONFIG_NRF_CLOUD_SEND_DEVICE_INFO_BOOTLOADER_VERSION=y/' \
		-e 's/^CONFIG_BOOTLOADER_MCUBOOT=y/# CONFIG_BOOTLOADER_MCUBOOT=y/' \
		-e 's/^CONFIG_MCUBOOT_IMG_MANAGER=y/# CONFIG_MCUBOOT_IMG_MANAGER=y/' \
		app/prj.conf
	@touch $@

clone-assettracker: $(BASE_DIR)/assettracker/.cloned

build-assettracker: $(BASE_DIR)/assettracker/.cloned
	@echo "=== Building Asset-Tracker-Template for $(ASSETTRACKER_BOARD) ==="
	cd $(BASE_DIR)/assettracker/source && \
		ZEPHYR_BASE=$(ASSETTRACKER_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(ASSETTRACKER_VENV)/bin:$$PATH" \
		west build --pristine --build-dir build app \
		--board $(ASSETTRACKER_BOARD) --no-sysbuild -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON || true

analyze-assettracker: build-assettracker build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing Asset-Tracker-Template ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/assettracker/source/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/assettracker-analysis.json \
		--app-source-prefix=$(ASSETTRACKER_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS)

clean-assettracker:
	rm -rf $(BASE_DIR)/assettracker/source/build

# ============================================================
# ThingSet SDK — IoT protocol SDK with multi-transport support
# Repo: ThingSet/thingset-zephyr-sdk (commit 8b32f7db)
# Zephyr v4.4-branch (SDK 1.0.1 compatible, no patches needed)
# Board: native_sim
# Sample: samples/counter (has native_sim board config)
# Threads: ThingSet SDK threads + system threads
# ============================================================

$(BASE_DIR)/thingset/.cloned:
	@echo "=== Cloning ThingSet SDK ==="
	mkdir -p $(BASE_DIR)/thingset
	python3 -m venv $(THINGSET_VENV)
	$(THINGSET_VENV)/bin/pip install --upgrade pip west
	if [ ! -d $(BASE_DIR)/thingset/source/.git ]; then \
		git clone https://github.com/ThingSet/thingset-zephyr-sdk.git $(BASE_DIR)/thingset/source; \
	fi
	cd $(BASE_DIR)/thingset/source && git checkout $(THINGSET_COMMIT)
	if [ ! -f $(BASE_DIR)/thingset/.west/config ]; then \
		cd $(BASE_DIR)/thingset/source && unset ZEPHYR_BASE && PATH="$(THINGSET_VENV)/bin:$$PATH" west init -l .; \
	fi
	cd $(BASE_DIR)/thingset/source && unset ZEPHYR_BASE && PATH="$(THINGSET_VENV)/bin:$$PATH" west update
	cd $(BASE_DIR)/thingset && PATH="$(THINGSET_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@echo "=== Installing esptool for ESP32-C3 build ==="
	$(THINGSET_VENV)/bin/pip install -q "esptool>=5.0.2"
	@echo "=== Fetching Espressif HAL blobs ==="
	cd $(BASE_DIR)/thingset/source && unset ZEPHYR_BASE && PATH="$(THINGSET_VENV)/bin:$$PATH" west blobs fetch hal_espressif
	@touch $@

clone-thingset: $(BASE_DIR)/thingset/.cloned

build-thingset: $(BASE_DIR)/thingset/.cloned
	@echo "=== Building ThingSet SDK serial_bluetooth_gateway sample for $(THINGSET_BOARD) ==="
	cd $(BASE_DIR)/thingset/source && \
		ZEPHYR_BASE=$(THINGSET_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(THINGSET_VENV)/bin:$$PATH" \
		west build --pristine --build-dir build samples/serial_bluetooth_gateway \
		--board $(THINGSET_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON || true

analyze-thingset: build-thingset build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing ThingSet SDK ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/thingset/source/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/thingset-analysis.json \
		--app-source-prefix=$(THINGSET_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(RISCV_TARGET_FLAGS)

clean-thingset:
	rm -rf $(BASE_DIR)/thingset/source/build

# ============================================================
# BACnet-Zephyr — Building automation protocol stack
# Repo: bacnet-stack/bacnet-stack-zephyr (commit 52cd4aac)
# Zephyr v3.7.1, SDK 1.0.1 + version/toolchain patches
# Board: nucleo_f429zi (STM32F429ZI Cortex-M4)
# Sample: zephyr/samples/profiles/b-asc (BACnet Advanced Smart Controller)
# Threads: BACnet router/datalink + networking + shell + system threads
# ============================================================

$(BASE_DIR)/bacnet/.cloned:
	@echo "=== Cloning BACnet-Zephyr ==="
	mkdir -p $(BASE_DIR)/bacnet
	python3 -m venv $(BACNET_VENV)
	$(BACNET_VENV)/bin/pip install --upgrade pip west
	if [ ! -d $(BASE_DIR)/bacnet/source/.git ]; then \
		git clone https://github.com/bacnet-stack/bacnet-stack-zephyr.git $(BASE_DIR)/bacnet/source; \
	fi
	cd $(BASE_DIR)/bacnet/source && git checkout $(BACNET_COMMIT)
	if [ ! -f $(BASE_DIR)/bacnet/.west/config ]; then \
		cd $(BASE_DIR)/bacnet/source && unset ZEPHYR_BASE && PATH="$(BACNET_VENV)/bin:$$PATH" west init -l .; \
	fi
	cd $(BASE_DIR)/bacnet/source && unset ZEPHYR_BASE && PATH="$(BACNET_VENV)/bin:$$PATH" west update
	cd $(BASE_DIR)/bacnet && PATH="$(BACNET_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	@echo "=== Patching Zephyr v3.7.1 for SDK 1.0.1 compatibility ==="
	cd $(BASE_DIR)/bacnet && git -C zephyr checkout -- .
	cd $(BASE_DIR)/bacnet && patch -p1 -d zephyr < $(CURDIR)/src/bacnet/01-fix-sdk-version.patch
	cd $(BASE_DIR)/bacnet && patch -p1 -d zephyr < $(CURDIR)/src/bacnet/02-fix-toolchain-layout.patch
	@echo "=== Symlinking bacnet-stack to expected path ==="
	ln -sfn $(BASE_DIR)/bacnet/bacnet/stack $(BASE_DIR)/bacnet/source/stack
	@touch $@

clone-bacnet: $(BASE_DIR)/bacnet/.cloned

build-bacnet: $(BASE_DIR)/bacnet/.cloned
	@echo "=== Building BACnet b-asc sample for $(BACNET_BOARD) ==="
	cd $(BASE_DIR)/bacnet/source && \
		ZEPHYR_BASE=$(BACNET_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(BACNET_VENV)/bin:$$PATH" \
		west build --pristine --build-dir build zephyr/samples/profiles/b-asc \
		--board $(BACNET_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DEXTRA_CONF_FILE=$(CURDIR)/src/bacnet/server-thread.conf || true

analyze-bacnet: build-bacnet build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing BACnet-Zephyr ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/bacnet/source/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/bacnet-analysis.json \
		--app-source-prefix=$(BACNET_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS) $(ARM_TARGET_FLAGS)

clean-bacnet:
	rm -rf $(BASE_DIR)/bacnet/source/build

# ============================================================
# libcsp — CubeSat Space Protocol
# Repo: libcsp/libcsp (develop branch, commit a47b4d73)
# No west.yml — workspace set up manually with Zephyr v4.4.0
# Board: native_sim
# Sample: contrib/zephyr/samples/server-client (router + server + client threads)
# Threads: router_task (K_THREAD_DEFINE), server_task, client_task + main
# ============================================================

$(BASE_DIR)/libcsp/.cloned:
	@echo "=== Cloning libcsp ==="
	mkdir -p $(BASE_DIR)/libcsp
	python3 -m venv $(LIBCSP_VENV)
	$(LIBCSP_VENV)/bin/pip install --upgrade pip west
	if [ ! -d $(BASE_DIR)/libcsp/zephyr/.git ]; then \
		unset ZEPHYR_BASE && PATH="$(LIBCSP_VENV)/bin:$$PATH" \
			west init -m https://github.com/zephyrproject-rtos/zephyr --mr v4.4.0 $(BASE_DIR)/libcsp; \
	fi
	cd $(BASE_DIR)/libcsp && unset ZEPHYR_BASE && PATH="$(LIBCSP_VENV)/bin:$$PATH" west update
	cd $(BASE_DIR)/libcsp && PATH="$(LIBCSP_VENV)/bin:$$PATH" pip install -q -r zephyr/scripts/requirements.txt
	if [ ! -d $(BASE_DIR)/libcsp/source/.git ]; then \
		git clone -b develop https://github.com/libcsp/libcsp.git $(BASE_DIR)/libcsp/source; \
	fi
	cd $(BASE_DIR)/libcsp/source && git checkout $(LIBCSP_COMMIT)
	@touch $@

clone-libcsp: $(BASE_DIR)/libcsp/.cloned

build-libcsp: $(BASE_DIR)/libcsp/.cloned
	@echo "=== Building libcsp server-client sample for $(LIBCSP_BOARD) ==="
	cd $(BASE_DIR)/libcsp/source && \
		ZEPHYR_BASE=$(LIBCSP_ZEPHYR) \
		ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK) \
		PATH="$(LIBCSP_VENV)/bin:$$PATH" \
		west build --pristine --build-dir build contrib/zephyr/samples/server-client \
		--board $(LIBCSP_BOARD) -- \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DZEPHYR_EXTRA_MODULES=$(BASE_DIR)/libcsp/source || true

analyze-libcsp: build-libcsp build-analyzer
	@mkdir -p $(ANALYSIS_DIR)
	@echo "=== Analyzing libcsp ==="
	$(ANALYZER) \
		--use-compdb $(BASE_DIR)/libcsp/source/build/compile_commands.json \
		--output-file $(ANALYSIS_DIR)/libcsp-analysis.json \
		--app-source-prefix=$(LIBCSP_APP_PREFIX) \
		$(ANALYZER_FLAGS) $(MINCUT_FLAGS)

clean-libcsp:
	rm -rf $(BASE_DIR)/libcsp/source/build

# ============================================================
# Aggregate targets
# ============================================================
clone: clone-zswatch clone-zmk clone-quadcopter clone-usp-zephyr clone-cannectivity clone-ecfw clone-opendeck clone-spinner clone-grbl clone-golioth clone-libresolar clone-sof clone-microros clone-findmycat clone-linaro clone-akiraos clone-assettracker clone-thingset clone-bacnet clone-libcsp
build: build-zswatch build-zmk build-quadcopter build-usp-zephyr build-cannectivity build-ecfw build-opendeck build-spinner build-grbl build-golioth build-libresolar build-sof build-microros build-findmycat build-linaro build-akiraos build-assettracker build-thingset build-bacnet build-libcsp

# All per-project analyze targets, used by the parallel aggregate below.
ANALYZE_TARGETS := analyze-zswatch analyze-zmk analyze-quadcopter analyze-usp-zephyr \
	analyze-cannectivity analyze-ecfw analyze-opendeck analyze-spinner analyze-grbl \
	analyze-golioth analyze-libresolar analyze-sof analyze-microros analyze-findmycat \
	analyze-linaro analyze-akiraos analyze-assettracker analyze-thingset analyze-bacnet \
	analyze-libcsp

# Runs all per-project analyses in parallel (JOBS at a time, default nproc).
# Override with `make analyze-all JOBS=1` for sequential, `JOBS=20` for max.
analyze:
	$(MAKE) -j$(JOBS) $(ANALYZE_TARGETS)

analyze-all:
	$(MAKE) -j$(JOBS) $(ANALYZE_TARGETS)

clean: clean-zswatch clean-zmk clean-quadcopter clean-usp-zephyr clean-cannectivity clean-ecfw clean-opendeck clean-spinner clean-grbl clean-golioth clean-libresolar clean-sof clean-microros clean-findmycat clean-linaro clean-akiraos clean-assettracker clean-thingset clean-bacnet clean-libcsp clean-analyzer

clean-analyzer:
	rm -rf $(CURDIR)/analyzer/build

distclean:
	rm -rf $(BASE_DIR)/cannectivity $(BASE_DIR)/ecfw $(BASE_DIR)/opendeck $(BASE_DIR)/spinner $(BASE_DIR)/zswatch $(BASE_DIR)/zmk $(BASE_DIR)/quadcopter $(BASE_DIR)/usp-zephyr $(BASE_DIR)/grbl $(BASE_DIR)/golioth $(BASE_DIR)/libresolar $(BASE_DIR)/sof $(BASE_DIR)/microros $(BASE_DIR)/findmycat $(BASE_DIR)/linaro $(BASE_DIR)/akiraos $(BASE_DIR)/assettracker $(BASE_DIR)/thingset $(BASE_DIR)/bacnet $(BASE_DIR)/libcsp
	rm -rf $(ANALYSIS_DIR)
	rm -rf $(CURDIR)/analyzer/build
