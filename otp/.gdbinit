target remote localhost:2331
monitor flash device = LPC1830
monitor flash download = 1
load ../out/Release/tessel-otp.elf

monitor reset 3
shell sleep 1

monitor reg r13 = (0x10000000)
monitor reg pc = (0x10000004)
monitor reg r13
monitor reg pc

continue