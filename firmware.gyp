{
  'variables': {
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
      '__TESSEL_RUNTIME_VERSION__="<!(git --git-dir deps/runtime/.git log --pretty=format:\'%h\' -n 1)"',
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

      '-include <!(pwd)/deps/runtime/deps/axtls/config/embed/config.h',

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
        'builtin/tessel.js'
      ],
      'rules': [
        {
          'rule_name': 'build-tessel',
          'extension': 'js',
          'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/<(RULE_INPUT_ROOT).c' ],
          'action': [ './tools/compile_js.sh', '<(RULE_INPUT_PATH)', '<@(_outputs)' ]
        }
      ]
    },

    {
      'target_name': 'wifi-cc3000-builtin',
      'type': 'none',
      'sources': [
        'builtin/wifi-cc3000.js'
      ],
      'rules': [
        {
          'rule_name': 'build-wifi-cc3000',
          'extension': 'js',
          'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/<(RULE_INPUT_ROOT).c' ],
          'action': [ './tools/compile_js.sh', '<(RULE_INPUT_PATH)', '<@(_outputs)' ]
        }
      ]
    },

    {
      'target_name': 'cmsis-lpc18xx',
      'type': 'static_library',
      'sources': [
        'lpc18xx/Drivers/source/Font5x7.c',
        'lpc18xx/Drivers/source/LCDTerm.c',
        'lpc18xx/Drivers/source/lpc18xx_adc.c',
        'lpc18xx/Drivers/source/lpc18xx_atimer.c',
        'lpc18xx/Drivers/source/lpc18xx_can.c',
        'lpc18xx/Drivers/source/lpc18xx_cgu.c',
        'lpc18xx/Drivers/source/lpc18xx_dac.c',
        'lpc18xx/Drivers/source/lpc18xx_emc.c',
        'lpc18xx/Drivers/source/lpc18xx_evrt.c',
        'lpc18xx/Drivers/source/lpc18xx_gpdma.c',
        'lpc18xx/Drivers/source/lpc18xx_gpio.c',
        'lpc18xx/Drivers/source/lpc18xx_i2c.c',
        'lpc18xx/Drivers/source/lpc18xx_i2s.c',
        'lpc18xx/Drivers/source/lpc18xx_lcd.c',
        'lpc18xx/Drivers/source/lpc18xx_mcpwm.c',
        'lpc18xx/Drivers/source/lpc18xx_nvic.c',
        'lpc18xx/Drivers/source/lpc18xx_pwr.c',
        'lpc18xx/Drivers/source/lpc18xx_qei.c',
        'lpc18xx/Drivers/source/lpc18xx_rgu.c',
        'lpc18xx/Drivers/source/lpc18xx_rit.c',
        'lpc18xx/Drivers/source/lpc18xx_rtc.c',
        'lpc18xx/Drivers/source/lpc18xx_sct.c',
        'lpc18xx/Drivers/source/lpc18xx_scu.c',
        'lpc18xx/Drivers/source/lpc18xx_sdif.c',
        'lpc18xx/Drivers/source/lpc18xx_sdmmc.c',
        'lpc18xx/Drivers/source/lpc18xx_ssp.c',
        'lpc18xx/Drivers/source/lpc18xx_timer.c',
        'lpc18xx/Drivers/source/lpc18xx_uart.c',
        'lpc18xx/Drivers/source/lpc18xx_wwdt.c',
        'lpc18xx/otp/otprom.c',
      ],
      'include_dirs': [
        'lpc18xx/Drivers/include',
        'lpc18xx/Core/CMSIS/Include',
        'lpc18xx/Core/Include',
        'lpc18xx/Core/Device/NXP/LPC18xx/Include',
        'lpc18xx/otp',
        'src/sys',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'lpc18xx/Drivers/include',
          'lpc18xx/Core/CMSIS/Include',
          'lpc18xx/Core/Include',
          'lpc18xx/Core/Device/NXP/LPC18xx/Include',
          'lpc18xx/otp',
          'src/sys',
        ],
      },
    },

    {
      'target_name': 'usb',
      'type': 'static_library',
      'sources': [
        'deps/usb/usb_requests.c',
        'deps/usb/lpc18_43/usb_lpc18_43.c',
        'deps/usb/class/dfu/dfu.c',
      ],
      'include_dirs': [
        'deps/usb',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'deps/usb',
          'deps/usb/lpc18_43',
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
        'CC3000_DEBUG',
        'SEND_NON_BLOCKING',
        'TESSEL_FASTCONNECT=1'
      ],
      'sources': [
        # Startup script
        'src/sys/startup_lpc1800.s',

        '<(SHARED_INTERMEDIATE_DIR)/tessel.c',
        '<(SHARED_INTERMEDIATE_DIR)/wifi-cc3000.c',
        'src/variants/lpc18xx/variant.c',

        'src/tm/tm_net.c',
        'src/tm/tm_random.c',
        'src/tm/tm_uptime.c',
        'src/tm/tm_timestamp.c',

        'src/sdram/sdram_init.c',
        'src/cc3000/host_spi.c',

        'src/hw/hw_analog.c',
        'src/hw/hw_highspeedsignal.c',
        'src/hw/hw_digital.c',
        'src/hw/hw_i2c.c',
        'src/hw/hw_interrupt.c',
        'src/hw/hw_net.c',
        'src/hw/hw_pwm.c',
        'src/hw/hw_wait.c',
        'src/hw/hw_spi.c',
        'src/hw/hw_spi_async.c',
        'src/hw/hw_uart.c',
        'src/hw/hw_gpdma.c',
        'src/hw/l_hw.c',

        'src/sys/sbrk.c',
        'src/sys/spi_flash.c',
        'src/sys/clock.c',
        'src/sys/startup.c',
        'src/sys/system_lpc18xx.c',
        'src/sys/bootloader.c',

        'src/test/test.c',
        'src/test/test_hw.c',
        'src/test/testalator.c',

        'src/cc3000/utility/cc3000_common.c',
        'src/cc3000/utility/evnt_handler.c',
        'src/cc3000/utility/hci.c',
        'src/cc3000/utility/netapp.c',
        'src/cc3000/utility/nvmem.c',
        'src/cc3000/utility/security.c',
        'src/cc3000/utility/socket.c',
        'src/cc3000/utility/wlan.c',

        'src/main.c',
        'src/syscalls.c',
        'src/tessel.c',
        'src/tessel_wifi.c',

        'src/usb/usb.c',
        'src/usb/device.c',
        'src/usb/log_interface.c',
        'src/usb/msg_interface.c',

        
        'src/addons/neopixel.c'
      ],
      'dependencies': [
        'deps/runtime/libtm.gyp:libtm',
        'deps/runtime/libtm.gyp:axtls',
        'deps/runtime/libcolony.gyp:libcolony',
        'cmsis-lpc18xx',
        'tessel-builtin',
        'wifi-cc3000-builtin',
        'usb',
      ],
      'libraries': [
        '-lm',
        '-lc',
        '-lnosys',
      ],
      'ldflags': [
        '-T<!(pwd)/src/ldscript_rom_gnu.ld',
        '-lm',
        '-lc',
        '-lnosys',
        '-Wl,--gc-sections', '-Wl,-marmelf',
      ],
      'include_dirs': [
        'deps/runtime/src',
        'deps/runtime/deps/colony-lua/src',
        'deps/runtime/deps/axtls/config',
        'deps/runtime/deps/axtls/ssl',
        'deps/runtime/deps/axtls/crypto',
        'src',
        'src/cc3000',
        'src/cc3000/utility',
        'src/hw',
        'src/sdram',
        'src/sys',
        'src/test',
        'src/tm',
        'src/variants/lpc18xx',
        'src/addons'
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
        'src/sys/startup_lpc1800.s',

        'src/variants/lpc18xx/variant.c',

        'src/hw/hw_digital.c',

        'src/tessel.c',
        'src/sys/sbrk.c',
        'src/sys/spi_flash.c',
        'src/sys/clock.c',
        'src/sys/startup.c',
        'src/sys/system_lpc18xx.c',
        'src/sys/bootloader.c',

        'boot/main.c',
        'boot/usb.c',
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
        '-T<!(pwd)/boot/ldscript_rom_gnu.ld',
        '-lm',
        '-lc',
        '-lnosys',
        '-Wl,--gc-sections', '-Wl,-marmelf',
      ],
      'include_dirs': [
        'boot',
        'src',
        'src/cc3000',
        'src/cc3000/utility',
        'src/hw',
        'src/sdram',
        'src/sys',
        'src/test',
        'src/tm',
        'src/variants/lpc18xx',
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
        'src/sys/startup_lpc1800.s',

        'src/variants/lpc18xx/variant.c',

        'src/hw/hw_digital.c',

        'src/sys/sbrk.c',
        'src/sys/spi_flash.c',
        'src/sys/clock.c',
        'src/sys/startup.c',
        'src/sys/system_lpc18xx.c',
        'src/sys/bootloader.c',

        'otp/main.c',

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
        '-T<!(pwd)/otp/ldscript_ram_gnu.ld',
        '-lm',
        '-lc',
        '-lnosys',
        '-Wl,--gc-sections', '-Wl,-marmelf',
      ],
      'include_dirs': [
        'otp',
        'src',
        'src/cc3000',
        'src/cc3000/utility',
        'src/hw',
        'src/sdram',
        'src/sys',
        'src/test',
        'src/tm',
        'src/variants/lpc18xx',
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
        'src/sys/startup_lpc1800.s',

        'src/variants/lpc18xx/variant.c',

        'src/hw/hw_digital.c',

        'src/sys/sbrk.c',
        'src/sys/spi_flash.c',
        'src/sys/clock.c',
        'src/sys/startup.c',
        'src/sys/system_lpc18xx.c',
        'src/sys/bootloader.c',

        'erase/main.c',

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
        '-T<!(pwd)/otp/ldscript_ram_gnu.ld',
        '-lm',
        '-lc',
        '-lnosys',
        '-Wl,--gc-sections', '-Wl,-marmelf',
      ],
      'include_dirs': [
        'otp',
        'src',
        'src/cc3000',
        'src/cc3000/utility',
        'src/hw',
        'src/sdram',
        'src/sys',
        'src/test',
        'src/tm',
        'src/variants/lpc18xx',
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
        'src/sys/startup_lpc1800.s',

        'src/variants/lpc18xx/variant.c',

        'src/hw/hw_digital.c',
        'src/cc3000/host_spi.c',
        'src/sys/sbrk.c',
        'src/sys/spi_flash.c',
        'src/sys/clock.c',
        'src/sys/startup.c',
        'src/sys/system_lpc18xx.c',
        'src/sys/bootloader.c',
        
        'src/cc3000/utility/cc3000_common.c',
        'src/cc3000/utility/evnt_handler.c',
        'src/cc3000/utility/hci.c',
        'src/cc3000/utility/netapp.c',
        'src/cc3000/utility/nvmem.c',
        'src/cc3000/utility/security.c',
        'src/cc3000/utility/socket.c',
        'src/cc3000/utility/wlan.c',
        

        'src/hw/hw_digital.c',
        'src/hw/hw_wait.c',
        'src/hw/hw_spi.c',

        'cc3k_patch/main.c',
        'cc3k_patch/PatchProgrammer.c'
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
        '-T<!(pwd)/cc3k_patch/ldscript_ram_gnu.ld',
        '-lm',
        '-lc',
        '-lnosys',
        '-Wl,--gc-sections', '-Wl,-marmelf',
      ],
      'include_dirs': [
        'cc3k_patch/',
        'src',
        'src/cc3000',
        'src/cc3000/utility',
        'src/hw',
        'src/sdram',
        'src/sys',
        'src/test',
        'src/tm',
        'src/variants/lpc18xx',
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