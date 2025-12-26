:
# ====================================================================================
# PicoPeanutGB build all script 
# Builds the emulator for the default and alternate configuration
# Binaries are copied to the releases folder
#   - piconesPlusPimoroniDV.uf2     Pimoroni Pico DV Demo Base
#   - piconesPlusAdaFruitDVISD.uf2  AdaFruit HDMI and SD Breakout boards
#   - piconesPlusFeatherDVI.uf2     Adafruit Feather RP2040 DVI + SD Wing
# ====================================================================================
cd `dirname $0` || exit 1
[ -d releases ] && rm -rf releases
mkdir releases || exit 1
# check picotool exists in path
if ! command -v picotool &> /dev/null
then
	echo "picotool could not be found"
	echo "Please install picotool from https://github.com/raspberrypi/picotool.git" 
	exit
fi
# build for Pico is disabled. RP2040 builds are too slow.
# HWCONFIGS="1 2 3 4"
# for HWCONFIG in $HWCONFIGS
# do	
# 	./bld.sh -c $HWCONFIG
# done
# build for Pico 2 arm and risc-v
# build for Pico 2 (w) -arm-s
# No pico2_w binaries for HWConfig 1 (#132)
# no pico_w binaries for HWConfig 2 (#136)
# No build for WaveShare RP2350-PiZero (#7)
HWCONFIGS="1 2 5 6 7 8 9 10 12 13"
for HWCONFIG in $HWCONFIGS
do
	./bld.sh -c $HWCONFIG -2 || exit 1
	# don't build for w when HWCONFIG=1, 5, 6, 7 and 8
	if [[ $HWCONFIG -eq 2 ]]; then
		./bld.sh -c $HWCONFIG -2 -w || exit 1
	fi
done
# # build for Pico 2 -riscv
# # No pico2_w binaries for HWConfig 1 (#132)
# # No risc binaries for Metro RP2350 and Fruit Jam (SD card not working)
# HWCONFIGS="1 2 7 9 10"
# for HWCONFIG in $HWCONFIGS
# do
# 	./bld.sh -c $HWCONFIG -r -t $PICO_SDK_PATH/toolchain/RISCV_RPI_2_0_0_2/bin || exit 1
# 	# don't build for w when HWCONFIG=1 (#132), 5 and 6, 7 and 9
# 	if [[ $HWCONFIG -ne 1 && $HWCONFIG -ne 5 && $HWCONFIG -ne 6 && $HWCONFIG -ne 7 && $HWCONFIG -ne 9  && $HWCONFIG -ne 10 ]]; then
# 	 	./bld.sh -c $HWCONFIG -r -t $PICO_SDK_PATH/toolchain/RISCV_RPI_2_0_0_2/bin -w || exit 1
# 	fi
# done	
if [ -z "$(ls -A releases)" ]; then
	echo "No UF2 files found in releases folder"
	exit 1
fi
for UF2 in releases/*.uf2
do
	ls -l $UF2
	picotool info $UF2
	echo " "
done

