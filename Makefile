ENABLE_TLS ?= 1
ENABLE_NET ?= 1
COLONY_STATE_CACHE ?= 0
COLONY_PRELOAD_ON_INIT ?= 0

CONFIG ?= Release

compile = \
	AR=arm-none-eabi-ar AR_host=arm-none-eabi-ar AR_target=arm-none-eabi-ar CC=arm-none-eabi-gcc CXX=arm-none-eabi-g++ gyp $(1) --depth=. -f ninja-arm -D builtin_section=.text -D COLONY_STATE_CACHE=$(COLONY_STATE_CACHE) -D COLONY_PRELOAD_ON_INIT=$(COLONY_PRELOAD_ON_INIT) -D enable_ssl=$(ENABLE_TLS) -D enable_net=$(ENABLE_NET) &&\
	ninja -C out/$(CONFIG)

.PHONY: all

all:

clean:
	ninja -v -C out/Debug -t clean
	ninja -v -C out/Release -t clean

nuke: 
	rm -rf out build

update:
	git submodule update --init --recursive
	npm install
	(cd deps/runtime; npm install)
	(cd test; npm install)


# Targets

firmware:
	$(call compile, firmware.gyp)

arm: firmware
	arm-none-eabi-size out/$(CONFIG)/tessel-firmware.elf out/$(CONFIG)/tessel-boot.elf out/$(CONFIG)/tessel-otp.elf

arm-deploy: arm deploy

deploy:
	tessel dfu-restore ./out/$(CONFIG)/tessel-firmware.bin
