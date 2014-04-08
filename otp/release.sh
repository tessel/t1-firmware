make clean
make -j4 BOARD_V=0
cp main.bin tm/tm_otp_v00.bin

make clean
make -j4 BOARD_V=2
cp main.bin tm/tm_otp_v02.bin