# Tessel Firmware

## API documentation

* [Tessel JavaScript APIs](https://github.com/tessel/docs)

## Directory structure

* `src/` -- The main firmware source, including the C bindings to the hardware
* `builtin/` -- JS binding to hardware specific API calls (such as I2C, SPI, UART, GPIO)
* `lpc18xx/` -- NXP drivers for the LPC1830 peripherals
* `boot/` -- USB DFU bootloader
* `otp/` -- Factory setup to set the board version and install the bootloader. Can be used to update or reinstall the bootloader
* `erase/` -- Tool to erase the JS bundle from the bootloader if a bug makes the main firmware unusable
* `cc3k_patch/` -- Tool to update the CC3000 WiFi firmware

## Compiling

### Dependencies

Building the firmware requires [gcc-arm-embedded](https://launchpad.net/gcc-arm-embedded), [gyp](https://code.google.com/p/gyp/), and [ninja](http://martine.github.io/ninja/).

#### OS X

To install quickly on a Mac with Brew:

```
brew tap tcr/tcr
brew install gcc-arm gyp ninja
```

#### Ubuntu 14.04

All dependencies are in the Ubuntu 14.04 repositories:

```
sudo apt-get install git nodejs npm nodejs-legacy gcc-arm-none-eabi gyp ninja-build
```

### Cloning the firmware repository

We'll clone the firmware repo itself into a folder called `tessel/firmware`. Inside your `tessel` folder, run:

```
git clone https://github.com/tessel/firmware
cd firmware
make update
```

### Updating

To update your firmware version, you can `git pull`. Each time you pull a new commit of firmware, update your submodules and npm packages by running `make update`.

### Compiling

Inside the `tessel/firmware/` directory, run

```
make arm
```

This will build a binary. If you have no errors, you should have a new binary file called `out/Release/tessel-firmware.bin`.

### Loading firmware onto Tessel

After building your firmware in the previous step:

1. [Put your tessel in dfu mode](https://github.com/tessel/beta/wiki#firmware-updates)
2. Run `tessel update ./out/Release/tessel-firmware.bin`

**Quick tip:** You can build and deploy firmware by DFU in one step by running `make arm-deploy`.

