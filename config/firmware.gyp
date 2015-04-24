{
  'variables': {
    'firmware_path': '../src',
    'lpc18xx_path': '../lpc18xx',
    'usb_path': '../deps/usb',
    'runtime_path': "../deps/runtime",
    'builtin_path': '../builtin',
    'tools_path': '../tools',
    'otp_path': '../otp',
    'erase_path': '../erase',
    'boot_path': '../boot',
    'cc3k_path': '../cc3k_patch',
    'COLONY_STATE_CACHE%': '0',
    'COLONY_PRELOAD_ON_INIT%': '0',
  },

  'target_defaults': {
    'defines': [
      'COLONY_EMBED',
      'CONFIG_PLATFORM_EMBED',
      'TM_FS_vfs',
      '__thumb2__=1',
      'GPIO_PIN_INT',
      '_POSIX_SOURCE',
      'REGEX_WCHAR=1',
      'COLONY_STATE_CACHE=<(COLONY_STATE_CACHE)',
      'COLONY_PRELOAD_ON_INIT=<(COLONY_PRELOAD_ON_INIT)',
      '__TESSEL_FIRMWARE_VERSION__="<!(git log --pretty=format:\'%h\' -n 1)"',
      '__TESSEL_RUNTIME_VERSION__="<!(git --git-dir <(runtime_path)/.git log --pretty=format:\'%h\' -n 1)"',
      '__TESSEL_RUNTIME_SEMVER__="<!(node -p \"require(\\\"<(runtime_path)/package.json\\\").version")"',
    ],
    'cflags': [
      '-mcpu=cortex-m3',
      '-mthumb',
      '-gdwarf-2',
      '-mtune=cortex-m3',
      '-march=armv7-m',
      '-mlong-calls',
      '-mfix-cortex-m3-ldrd',
      '-mapcs-frame',
      '-msoft-float',
      '-mno-sched-prolog',
      # '-fno-hosted',
      '-ffunction-sections',
      '-fdata-sections',
      # '-fpermissive',

      '-include "<!(pwd)/<(runtime_path)/deps/axtls/config/embed/config.h"',

      '-Wall',
      '-Werror',
    ],
    'cflags_c': [
      '-std=gnu99',
    ],
    'ldflags': [
      '-mcpu=cortex-m3',
      '-mthumb',
      '-mtune=cortex-m3',
      '-march=armv7-m',
      '-mlong-calls',
      '-mfix-cortex-m3-ldrd',
      '-mapcs-frame',
      '-msoft-float',
      '-mno-sched-prolog',
      # '-fno-hosted',
      '-ffunction-sections',
      '-fdata-sections',
      # '-fpermissive',
      '-std=c99',

      '-Wall',
      '-Werror',
    ],

    'default_configuration': 'Release',
    'configurations': {
      'Debug': {
        'cflags': [
          '-gdwarf-2',
          '-O0',
        ],
        'ldflags': [
          '-gdwarf-2',
          '-O0',
        ]
      },
      'Release': {
        'cflags': [
          '-Ofast',
        ]
      }
    },
  },

  "targets": [

    {
      'target_name': 'tessel-builtin',
      'type': 'none',
      'sources': [
        '<(builtin_path)/tessel.js'
      ],
      'rules': [
        {
          'rule_name': 'build-tessel',
          'extension': 'js',
          'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/<(RULE_INPUT_ROOT).c' ],
          'action': [ '<(tools_path)/compile_js.sh', '<(RULE_INPUT_PATH)', '<@(_outputs)' ]
        }
      ]
    },

    {
      'target_name': 'wifi-cc3000-builtin',
      'type': 'none',
      'sources': [
        '<(builtin_path)/wifi-cc3000.js'
      ],
      'rules': [
        {
          'rule_name': 'build-wifi-cc3000',
          'extension': 'js',
          'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/<(RULE_INPUT_ROOT).c' ],
          'action': [ '<(tools_path)/compile_js.sh', '<(RULE_INPUT_PATH)', '<@(_outputs)' ]
        }
      ]
    },

    {
      'target_name': 'neopixels-builtin',
      'type': 'none',
      'sources': [
        '<(builtin_path)/neopixels.js'
      ],
      'rules': [
        {
          'rule_name': 'build-neopixels',
          'extension': 'js',
          'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/<(RULE_INPUT_ROOT).c' ],
          'action': [ '<(tools_path)/compile_js.sh', '<(RULE_INPUT_PATH)', '<@(_outputs)' ]
        }
      ]
    },

    {
      'target_name': 'cmsis-lpc18xx',
      'type': 'static_library',
      'sources': [
        '<(lpc18xx_path)/Drivers/source/Font5x7.c',
        '<(lpc18xx_path)/Drivers/source/LCDTerm.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_adc.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_atimer.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_can.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_cgu.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_dac.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_emc.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_evrt.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_gpdma.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_gpio.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_i2c.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_i2s.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_lcd.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_mcpwm.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_nvic.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_pwr.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_qei.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_rgu.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_rit.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_rtc.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_sct.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_scu.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_sdif.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_sdmmc.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_ssp.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_timer.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_uart.c',
        '<(lpc18xx_path)/Drivers/source/lpc18xx_wwdt.c',
        '<(lpc18xx_path)/otp/otprom.c',
      ],
      'include_dirs': [
        '<(lpc18xx_path)/Drivers/include',
        '<(lpc18xx_path)/Core/CMSIS/Include',
        '<(lpc18xx_path)/Core/Include',
        '<(lpc18xx_path)/Core/Device/NXP/LPC18xx/Include',
        '<(lpc18xx_path)/otp',
        '<(firmware_path)/sys',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(lpc18xx_path)/Drivers/include',
          '<(lpc18xx_path)/Core/CMSIS/Include',
          '<(lpc18xx_path)/Core/Include',
          '<(lpc18xx_path)/Core/Device/NXP/LPC18xx/Include',
          '<(lpc18xx_path)/otp',
          '<(firmware_path)/sys',
        ],
      },
    },

    {
      'target_name': 'usb',
      'type': 'static_library',
      'sources': [
        '<(usb_path)/usb_requests.c',
        '<(usb_path)/lpc18_43/usb_lpc18_43.c',
        '<(usb_path)/class/dfu/dfu.c',
      ],
      'include_dirs': [
        '<(usb_path)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(usb_path)',
          '<(usb_path)/lpc18_43',
        ],
      },
      'dependencies': [
        'cmsis-lpc18xx',
      ],
    },

    {
      'target_name': 'tessel-firmware-elf',
      'type': 'executable',
      'product_name': 'tessel-firmware.elf',
      'cflags': [
        '-Wall',
        '-Wextra',
        '-Werror',
      ],
      'defines': [
        'CC3000_DEBUG=1',
        'SEND_NON_BLOCKING',
        'TESSEL_FASTCONNECT=1',
        'CC3K_TIMEOUT=1'
      ],
      'sources': [
        # Startup script
        '<(firmware_path)/sys/startup_lpc1800.s',

        '<(SHARED_INTERMEDIATE_DIR)/tessel.c',
        '<(SHARED_INTERMEDIATE_DIR)/wifi-cc3000.c',
        '<(SHARED_INTERMEDIATE_DIR)/neopixels.c',
        '<(firmware_path)/variants/lpc18xx/variant.c',

        '<(firmware_path)/tm/tm_net.c',
        '<(firmware_path)/tm/tm_random.c',
        '<(firmware_path)/tm/tm_uptime.c',
        '<(firmware_path)/tm/tm_timestamp.c',

        '<(firmware_path)/sdram/sdram_init.c',
        '<(firmware_path)/cc3000/host_spi.c',

        '<(firmware_path)/hw/hw_analog.c',
        '<(firmware_path)/hw/hw_highspeedsignal.c',
        '<(firmware_path)/hw/hw_digital.c',
        '<(firmware_path)/hw/hw_readpulse.c',
        '<(firmware_path)/hw/hw_i2c.c',
        '<(firmware_path)/hw/hw_interrupt.c',
        '<(firmware_path)/hw/hw_net.c',
        '<(firmware_path)/hw/hw_pwm.c',
        '<(firmware_path)/hw/hw_wait.c',
        '<(firmware_path)/hw/hw_spi.c',
        '<(firmware_path)/hw/hw_spi_async.c',
        '<(firmware_path)/hw/hw_uart.c',
        '<(firmware_path)/hw/hw_swuart.c',
        '<(firmware_path)/hw/hw_gpdma.c',
        '<(firmware_path)/hw/l_hw.c',

        '<(firmware_path)/sys/sbrk.c',
        '<(firmware_path)/sys/spi_flash.c',
        '<(firmware_path)/sys/clock.c',
        '<(firmware_path)/sys/startup.c',
        '<(firmware_path)/sys/system_lpc18xx.c',
        '<(firmware_path)/sys/bootloader.c',

        '<(firmware_path)/test/test.c',
        '<(firmware_path)/test/test_nmea.c',
        '<(firmware_path)/test/test_hw.c',
        '<(firmware_path)/test/testalator.c',

        '<(firmware_path)/cc3000/utility/cc3000_common.c',
        '<(firmware_path)/cc3000/utility/evnt_handler.c',
        '<(firmware_path)/cc3000/utility/hci.c',
        '<(firmware_path)/cc3000/utility/netapp.c',
        '<(firmware_path)/cc3000/utility/nvmem.c',
        '<(firmware_path)/cc3000/utility/security.c',
        '<(firmware_path)/cc3000/utility/socket.c',
        '<(firmware_path)/cc3000/utility/wlan.c',

        '<(firmware_path)/main.c',
        '<(firmware_path)/syscalls.c',
        '<(firmware_path)/tessel.c',
        '<(firmware_path)/tessel_wifi.c',

        '<(firmware_path)/usb/usb.c',
        '<(firmware_path)/usb/device.c',
        '<(firmware_path)/usb/log_interface.c',
        '<(firmware_path)/usb/msg_interface.c',

        '<(firmware_path)/module_shims/audio-vs1053b.c',
        '<(firmware_path)/module_shims/gps-a2235h.c',
        '<(firmware_path)/module_shims/gps-nmea.c',
        
        '<(firmware_path)/addons/neopixel.c'
      ],
      'dependencies': [
        '<(runtime_path)/config/libtm.gyp:libtm',
        '<(runtime_path)/config/libtm.gyp:axtls',
        '<(runtime_path)/config/libcolony.gyp:libcolony',
        'cmsis-lpc18xx',
        'tessel-builtin',
        'wifi-cc3000-builtin',
        'neopixels-builtin',
        'usb',
      ],
      'libraries': [
        '-lm',
        '-lc',
        '-lnosys',
      ],
      'ldflags': [
        '-T \'<!(pwd)/<(firmware_path)/ldscript_rom_gnu.ld\'',
        '-lm',
        '-lc',
        '-lnosys',
        '-Wl,--gc-sections', '-Wl,-marmelf',
      ],
      'include_dirs': [
        '<(runtime_path)/src',
        '<(runtime_path)/deps/colony-lua/src',
        '<(runtime_path)/deps/axtls/config',
        '<(runtime_path)/deps/axtls/ssl',
        '<(runtime_path)/deps/axtls/crypto',
        '<(firmware_path)/',
        '<(firmware_path)/cc3000',
        '<(firmware_path)/cc3000/utility',
        '<(firmware_path)/hw',
        '<(firmware_path)/sdram',
        '<(firmware_path)/sys',
        '<(firmware_path)/test',
        '<(firmware_path)/tm',
        '<(firmware_path)/variants/lpc18xx',
        '<(firmware_path)/module_shims',
        '<(firmware_path)/addons'
      ]
    },

    {
      "target_name": "tessel-firmware-hex",
      "type": "none",
      "sources": [
        "<(PRODUCT_DIR)/tessel-firmware.elf"
      ],
      "rules": [
        {
          "rule_name": "hex",
          "extension": "elf",
          "outputs": [
            "<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).hex"
          ],
          "action": [ "arm-none-eabi-objcopy", "-O", "ihex", "<(RULE_INPUT_PATH)", "<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).hex" ]
        }
      ]
    },

    {
      "target_name": "tessel-firmware-bin",
      "type": "none",
      "sources": [
        "<(PRODUCT_DIR)/tessel-firmware.elf"
      ],
      "rules": [
        {
          "rule_name": "bin",
          "extension": "elf",
          "outputs": [
            "<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).bin"
          ],
          "action": [ "arm-none-eabi-objcopy", "-O", "binary", "<(RULE_INPUT_PATH)", "<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).bin" ]
        }
      ]
    },

    {
      'target_name': 'tessel-boot-elf',
      'type': 'executable',
      'product_name': 'tessel-boot.elf',
      'cflags': [
        '-Wall',
        '-Wextra',
        '-Wno-error',
        '-Wno-main',
        '-Wno-unused-parameter',
        '-Wno-unused-function',
      ],
      'sources': [
        # Startup script
        '<(firmware_path)/sys/startup_lpc1800.s',

        '<(firmware_path)/variants/lpc18xx/variant.c',

        '<(firmware_path)/hw/hw_digital.c',

        '<(firmware_path)/tessel.c',
        '<(firmware_path)/sys/sbrk.c',
        '<(firmware_path)/sys/spi_flash.c',
        '<(firmware_path)/sys/clock.c',
        '<(firmware_path)/sys/startup.c',
        '<(firmware_path)/sys/system_lpc18xx.c',
        '<(firmware_path)/sys/bootloader.c',

        '<(boot_path)/main.c',
        '<(boot_path)/usb.c',
      ],
      'dependencies': [
        'cmsis-lpc18xx',
        'usb',
      ],
      'libraries': [
        '-lm',
        '-lc',
        '-lnosys',
      ],
      'ldflags': [
        '-T \'<!(pwd)/<(boot_path)/ldscript_rom_gnu.ld\'',
        '-lm',
        '-lc',
        '-lnosys',
        '-Wl,--gc-sections', '-Wl,-marmelf',
      ],
      'include_dirs': [
        '<(boot_path)/',
        '<(firmware_path)/',
        '<(firmware_path)/cc3000',
        '<(firmware_path)/cc3000/utility',
        '<(firmware_path)/hw',
        '<(firmware_path)/sdram',
        '<(firmware_path)/sys',
        '<(firmware_path)/test',
        '<(firmware_path)/tm',
        '<(firmware_path)/variants/lpc18xx',
      ]
    },

    {
      "target_name": "tessel-boot-bin",
      "type": "none",
      "sources": [
        "<(PRODUCT_DIR)/tessel-boot.elf"
      ],
      "rules": [
        {
          "rule_name": "bin",
          "extension": "elf",
          "outputs": [
            "<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).bin"
          ],
          "action": [ "arm-none-eabi-objcopy", "-O", "binary", "<(RULE_INPUT_PATH)", "<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).bin" ]
        }
      ]
    },

    {
      'target_name': 'tessel-otp-elf',
      'type': 'executable',
      'product_name': 'tessel-otp.elf',
      'cflags': [
        '-Wall',
        '-Wextra',
      ],
      'sources': [
        # Startup script
        '<(firmware_path)/sys/startup_lpc1800.s',

        '<(firmware_path)/variants/lpc18xx/variant.c',

        '<(firmware_path)/hw/hw_digital.c',

        '<(firmware_path)/sys/sbrk.c',
        '<(firmware_path)/sys/spi_flash.c',
        '<(firmware_path)/sys/clock.c',
        '<(firmware_path)/sys/startup.c',
        '<(firmware_path)/sys/system_lpc18xx.c',
        '<(firmware_path)/sys/bootloader.c',

        '<(otp_path)/main.c',

        '<(INTERMEDIATE_DIR)/boot_image.c'
      ],
      'actions': [
        {
          'action_name': 'boot_image',
          'inputs': [
            '<(PRODUCT_DIR)/tessel-boot.bin'
          ],
          'outputs': [
            '<(INTERMEDIATE_DIR)/boot_image.c'
          ],
          'action': ['sh', '-c', '(cd "<(PRODUCT_DIR)" > /dev/null; xxd -i tessel-boot.bin) > <(INTERMEDIATE_DIR)/boot_image.c'],
        }
      ],
      'dependencies': [
        'cmsis-lpc18xx',
      ],
      'libraries': [
        '-lm',
        '-lc',
        '-lnosys',
      ],
      'ldflags': [
        '-T \'<!(pwd)/<(otp_path)/ldscript_ram_gnu.ld\'',
        '-lm',
        '-lc',
        '-lnosys',
        '-Wl,--gc-sections', '-Wl,-marmelf',
      ],
      'include_dirs': [
        '<(otp_path)/',
        '<(firmware_path)/',
        '<(firmware_path)/cc3000',
        '<(firmware_path)/cc3000/utility',
        '<(firmware_path)/hw',
        '<(firmware_path)/sdram',
        '<(firmware_path)/sys',
        '<(firmware_path)/test',
        '<(firmware_path)/tm',
        '<(firmware_path)/variants/lpc18xx',
      ]
    },

    {
      "target_name": "tessel-otp-bin",
      "type": "none",
      "sources": [
        "<(PRODUCT_DIR)/tessel-otp.elf"
      ],
      "rules": [
        {
          "rule_name": "bin",
          "extension": "elf",
          "outputs": [
            "<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).bin"
          ],
          "action": [ "arm-none-eabi-objcopy", "-O", "binary", "<(RULE_INPUT_PATH)", "<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).bin" ]
        }
      ]
    },

    {
      'target_name': 'tessel-erase-elf',
      'type': 'executable',
      'product_name': 'tessel-erase.elf',
      'cflags': [
        '-Wall',
        '-Wextra',
      ],
      'sources': [
        # Startup script
        '<(firmware_path)/sys/startup_lpc1800.s',

        '<(firmware_path)/variants/lpc18xx/variant.c',

        '<(firmware_path)/hw/hw_digital.c',

        '<(firmware_path)/sys/sbrk.c',
        '<(firmware_path)/sys/spi_flash.c',
        '<(firmware_path)/sys/clock.c',
        '<(firmware_path)/sys/startup.c',
        '<(firmware_path)/sys/system_lpc18xx.c',
        '<(firmware_path)/sys/bootloader.c',

        '<(erase_path)/main.c',

      ],
      'dependencies': [
        'cmsis-lpc18xx',
      ],
      'libraries': [
        '-lm',
        '-lc',
        '-lnosys',
      ],
      'ldflags': [
        '-T \'<!(pwd)/<(otp_path)/ldscript_ram_gnu.ld\'',
        '-lm',
        '-lc',
        '-lnosys',
        '-Wl,--gc-sections', '-Wl,-marmelf',
      ],
      'include_dirs': [
        '<(otp_path)/',
        '<(firmware_path)/',
        '<(firmware_path)/cc3000',
        '<(firmware_path)/cc3000/utility',
        '<(firmware_path)/hw',
        '<(firmware_path)/sdram',
        '<(firmware_path)/sys',
        '<(firmware_path)/test',
        '<(firmware_path)/tm',
        '<(firmware_path)/variants/lpc18xx',
      ]
    },

    {
      "target_name": "tessel-cc3k-patch-elf",
      'type': 'executable',
      'product_name': 'tessel-cc3k-patch.elf',
      'cflags': [
        '-Wall',
        '-Wextra',
      ],
      'sources': [
        # Startup script
        '<(firmware_path)/sys/startup_lpc1800.s',

        '<(firmware_path)/variants/lpc18xx/variant.c',

        '<(firmware_path)/hw/hw_digital.c',
        '<(firmware_path)/cc3000/host_spi.c',
        '<(firmware_path)/sys/sbrk.c',
        '<(firmware_path)/sys/spi_flash.c',
        '<(firmware_path)/sys/clock.c',
        '<(firmware_path)/sys/startup.c',
        '<(firmware_path)/sys/system_lpc18xx.c',
        '<(firmware_path)/sys/bootloader.c',
        
        '<(firmware_path)/cc3000/utility/cc3000_common.c',
        '<(firmware_path)/cc3000/utility/evnt_handler.c',
        '<(firmware_path)/cc3000/utility/hci.c',
        '<(firmware_path)/cc3000/utility/netapp.c',
        '<(firmware_path)/cc3000/utility/nvmem.c',
        '<(firmware_path)/cc3000/utility/security.c',
        '<(firmware_path)/cc3000/utility/socket.c',
        '<(firmware_path)/cc3000/utility/wlan.c',
        

        '<(firmware_path)/hw/hw_digital.c',
        '<(firmware_path)/hw/hw_wait.c',
        '<(firmware_path)/hw/hw_spi.c',

        '<(cc3k_path)/main.c',
        '<(cc3k_path)/PatchProgrammer.c'
      ],
      'dependencies': [
        'cmsis-lpc18xx',
      ],
      'libraries': [
        '-lm',
        '-lc',
        '-lnosys',
      ],
      'ldflags': [
        '-T \'<!(pwd)/<(cc3k_path)/ldscript_ram_gnu.ld\'',
        '-lm',
        '-lc',
        '-lnosys',
        '-Wl,--gc-sections', '-Wl,-marmelf',
      ],
      'include_dirs': [
        '<(cc3k_path)/',
        '<(firmware_path)/',
        '<(firmware_path)/cc3000',
        '<(firmware_path)/cc3000/utility',
        '<(firmware_path)/hw',
        '<(firmware_path)/sdram',
        '<(firmware_path)/sys',
        '<(firmware_path)/test',
        '<(firmware_path)/tm',
        '<(firmware_path)/variants/lpc18xx',
      ]
    },

    {
      "target_name": "tessel-cc3k-patch-bin",
      "type": "none",
      "sources": [
        "<(PRODUCT_DIR)/tessel-cc3k-patch.elf"
      ],
      "rules": [
        {
          "rule_name": "bin",
          "extension": "elf",
          "outputs": [
            "<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).bin"
          ],
          "action": [ "arm-none-eabi-objcopy", "-O", "binary", "<(RULE_INPUT_PATH)", "<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).bin" ]
        }
      ]
    },

    {
      "target_name": "tessel-erase-bin",
      "type": "none",
      "sources": [
        "<(PRODUCT_DIR)/tessel-erase.elf"
      ],
      "rules": [
        {
          "rule_name": "bin",
          "extension": "elf",
          "outputs": [
            "<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).bin"
          ],
          "action": [ "arm-none-eabi-objcopy", "-O", "binary", "<(RULE_INPUT_PATH)", "<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).bin" ]
        }
      ]
    },



    {
      "target_name": "tessel-firmware",
      "type": "none",
      "dependencies": [
        'tessel-firmware-elf',
        'tessel-firmware-hex',
        'tessel-firmware-bin',
        'tessel-boot-elf',
        'tessel-boot-bin',
        'tessel-otp-elf',
        'tessel-otp-bin',
        'tessel-erase-elf',
        'tessel-erase-bin',
        'tessel-cc3k-patch-elf',
        'tessel-cc3k-patch-bin'
      ]
    },
  ]
}
