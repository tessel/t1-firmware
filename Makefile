# Let's shortcut all the things

arm : TARGET ?= tessel-firmware
COLONY_STATE_CACHE ?= 0
COLONY_PRELOAD_ON_INIT ?= 0

CONFIG ?= Release

.PHONY: all

all:

arm:
	AS=arm-none-eabi-as CXX=arm-none-eabi-g++ CXX=arm-none-eabi-g++ AR=arm-none-eabi-ar AR_host=arm-none-eabi-ar AR_target=arm-none-eabi-ar CC=arm-none-eabi-gcc gyp firmware.gyp --depth=. -f ninja-arm -R $(TARGET) -D builtin_section=.text -D COLONY_STATE_CACHE=$(COLONY_STATE_CACHE) -D COLONY_PRELOAD_ON_INIT=$(COLONY_PRELOAD_ON_INIT)
	ninja -C out/$(CONFIG)
	arm-none-eabi-size out/$(CONFIG)/tessel-firmware.elf out/$(CONFIG)/tessel-boot.elf out/$(CONFIG)/tessel-otp.elf

clean:
	ninja -v -C out/Debug -t clean
	ninja -v -C out/Release -t clean

arm-deploy: arm deploy

deploy:
	tessel dfu-restore ./out/$(CONFIG)/tessel-firmware.bin

update:
	git submodule update --init --recursive
	npm install
	cd deps/runtime; npm install

nuke: 
	rm -rf out