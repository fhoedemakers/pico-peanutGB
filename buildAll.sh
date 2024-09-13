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
# build for Pico 2
HWCONFIGS="1 2"
for HWCONFIG in $HWCONFIGS
do
	./bld.sh -c $HWCONFIG -2
done
if [ -z "$(ls -A releases)" ]; then
	echo "No UF2 files found in releases folder"
	exit
fi
for UF2 in releases/*.uf2
do
	ls -l $UF2
	picotool info $UF2
	echo " "
done
