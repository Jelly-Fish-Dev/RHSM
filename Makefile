# RHSM – Retro Handheld Save Manager
#
# On the host (macOS/Linux with Docker):
#   make                 cross-compile inside the toolchain container + stage dist/App/RHSM
#   make zip             release zip (unzip onto the SD card root)
#   make shell           interactive shell in the toolchain container
#   make debug           debug build (-O0 -g3, not stripped) staged into dist/App/RHSM
#   make deploy HOST=ip  copy dist/App/RHSM to a Mini Plus over SSH
#   make log HOST=ip     print the app's log.txt from the device
#   make debug-on HOST=ip / debug-off HOST=ip / gdb HOST=ip   remote debugging
#   make clean
#
# Add DEBUG=1 to any target (deploy, gdb, zip...) to use the debug build.
# Per-machine settings (device IP etc.) go in local.mk, which git ignores. Example:
#   RHSM_HOST ?= 192.168.1.50
#
# Inside the container the same Makefile does the real build (targets prefixed with _).

-include local.mk

APP        := RHSM
TARGET     := rhsm
DEBUG      ?= 0
VARIANT    := $(if $(filter 1,$(DEBUG)),debug,release)
VERSION    ?= $(shell git describe --tags --always --dirty 2>/dev/null || echo dev)$(if $(filter 1,$(DEBUG)),-debug)

DOCKER_IMAGE  ?= rhsm-toolchain:latest
TOOLCHAIN_DIR := /opt/miyoomini-toolchain
SYSROOT       := $(TOOLCHAIN_DIR)/usr/arm-linux-gnueabihf/sysroot
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/usr/bin/arm-linux-gnueabihf-
GDBSERVER_SRC := $(TOOLCHAIN_DIR)/arm-linux-gnueabihf/libc/usr/bin/gdbserver

BUILD_ROOT := build
BUILD_DIR  := $(BUILD_ROOT)/$(VARIANT)
DIST_DIR   := dist
APP_DIST  := $(DIST_DIR)/App/$(APP)

# Device side
HOST       ?= $(RHSM_HOST)
SSH_USER   ?= onion
DEVICE_APP := /mnt/SDCARD/App/$(APP)
SSH        := ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=accept-new $(SSH_USER)@$(HOST)

# Optional password login (RHSM_SSH_PASS in local.mk), answered by scripts/askpass.sh.
ifneq ($(RHSM_SSH_PASS),)
export RHSM_SSH_PASS
export SSH_ASKPASS := $(CURDIR)/scripts/askpass.sh
export SSH_ASKPASS_REQUIRE := force
export DISPLAY ?= :0
endif

DOCKER_RUN := docker run --rm --platform linux/amd64 \
	-v "$(CURDIR)":/root/workspace -w /root/workspace $(DOCKER_IMAGE)

.PHONY: all build dist debug zip shell clean deploy log debug-on debug-off gdb check-host \
	toolchain toolchain-rebuild _build _dist _gdbserver

ifeq ($(wildcard $(TOOLCHAIN_DIR)),)
# ---------------------------------------------------------------- host side

all: dist

# Build the toolchain image (docker/Dockerfile) if it isn't there yet.
toolchain:
	@docker image inspect $(DOCKER_IMAGE) >/dev/null 2>&1 || \
		docker build --platform linux/amd64 -t $(DOCKER_IMAGE) docker

toolchain-rebuild:
	docker build --platform linux/amd64 --pull -t $(DOCKER_IMAGE) docker

build dist: toolchain
	$(DOCKER_RUN) make -j4 DEBUG=$(DEBUG) VERSION=$(VERSION) _dist

debug:
	$(MAKE) DEBUG=1 dist

zip: dist
	cd $(DIST_DIR) && rm -f $(APP)-$(VERSION).zip && \
		zip -qr -X $(APP)-$(VERSION).zip App/$(APP) && echo "$(DIST_DIR)/$(APP)-$(VERSION).zip"

shell: toolchain
	docker run --rm -it --platform linux/amd64 \
		-v "$(CURDIR)":/root/workspace -w /root/workspace $(DOCKER_IMAGE) bash

check-host:
	@test -n "$(HOST)" || { echo "Set HOST=<device ip> (or export RHSM_HOST)"; exit 1; }

deploy: check-host dist
	COPYFILE_DISABLE=1 tar -C $(DIST_DIR) -cf - App/$(APP) | \
		$(SSH) 'tar -C /mnt/SDCARD -xf - && rm -f $(DEVICE_APP)/._* $(DEVICE_APP)/bin/._*'
	@echo "Deployed to $(HOST):$(DEVICE_APP)"

log: check-host
	$(SSH) 'cat $(DEVICE_APP)/log.txt'

$(BUILD_ROOT)/tools/gdbserver: | toolchain
	$(DOCKER_RUN) make _gdbserver

debug-on: check-host $(BUILD_ROOT)/tools/gdbserver
	cat $(BUILD_ROOT)/tools/gdbserver | $(SSH) \
		'cat > $(DEVICE_APP)/bin/gdbserver && chmod +x $(DEVICE_APP)/bin/gdbserver && touch $(DEVICE_APP)/debug'
	@echo "Debug mode on: launch RHSM on the device, then run: make gdb$(if $(filter 1,$(DEBUG)), DEBUG=1)"

debug-off: check-host
	$(SSH) 'rm -f $(DEVICE_APP)/debug $(DEVICE_APP)/bin/gdbserver'

gdb: check-host toolchain
	docker run --rm -it --platform linux/amd64 \
		-v "$(CURDIR)":/root/workspace -w /root/workspace $(DOCKER_IMAGE) \
		$(CROSS_COMPILE)gdb -q $(BUILD_DIR)/$(TARGET) \
			-ex "set sysroot $(SYSROOT)" \
			-ex "target remote $(HOST):2345" \
			-x scripts/rhsm.gdb

clean:
	rm -rf $(BUILD_ROOT) $(DIST_DIR)

else
# ----------------------------------------------------------- container side

CC    := $(CROSS_COMPILE)gcc
STRIP := $(CROSS_COMPILE)strip

ARCH_FLAGS := -marm -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
ifeq ($(DEBUG),1)
OPT_FLAGS := -O0 -g3 -DDEBUG
else
OPT_FLAGS := -O2 -g -DNDEBUG
endif

# -Wno-format-truncation: snprintf truncating an over-long path is intended (never overflows).
CFLAGS  := -std=gnu99 $(OPT_FLAGS) -Wall -Wextra -Wno-format-truncation $(ARCH_FLAGS) \
	-Isrc -Ithird_party/sqlite -I$(SYSROOT)/usr/include \
	-DPLATFORM_MIYOOMINI -DRHSM_VERSION=\"$(VERSION)\" -MMD -MP
LDFLAGS := -L$(SYSROOT)/usr/lib -Wl,-rpath-link,$(SYSROOT)/usr/lib
LDLIBS  := -lSDL_image -lSDL_ttf -lSDL -lpthread -lm

# Vendored SQLite: always optimised, no warnings, single-threaded, no extensions.
SQLITE_CFLAGS := -std=gnu99 -O2 -g -w $(ARCH_FLAGS) -MMD -MP \
	-DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DEFAULT_MEMSTATUS=0 \
	-DSQLITE_DQS=0 -DSQLITE_OMIT_DEPRECATED -DSQLITE_DEFAULT_FOREIGN_KEYS=1

SRCS := $(shell find src -name '*.c') third_party/sqlite/sqlite3.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/obj/%.o)

$(BUILD_DIR)/obj/third_party/sqlite/sqlite3.o: CFLAGS := $(SQLITE_CFLAGS)

all: _dist

_build: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

_dist: _build
	rm -rf $(APP_DIST)
	mkdir -p $(APP_DIST)/bin $(APP_DIST)/res
	cp app/config.json app/launch.sh app/icon.png $(APP_DIST)/
	chmod +x $(APP_DIST)/launch.sh
	cp -r res/fonts $(APP_DIST)/res/
ifeq ($(DEBUG),1)
	cp $(BUILD_DIR)/$(TARGET) $(APP_DIST)/bin/$(TARGET)
else
	$(STRIP) -o $(APP_DIST)/bin/$(TARGET) $(BUILD_DIR)/$(TARGET)
endif
	@echo "Staged $(APP_DIST) ($(VERSION), $(VARIANT))"

_gdbserver:
	mkdir -p $(BUILD_ROOT)/tools
	cp $(GDBSERVER_SRC) $(BUILD_ROOT)/tools/gdbserver
	$(STRIP) $(BUILD_ROOT)/tools/gdbserver

-include $(OBJS:.o=.d)

endif
