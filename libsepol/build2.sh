#export CROSS=~/Downloads/android-ndk-r11c/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-
#export CROSS=/home/yannik/Downloads/android-ndk-r11c/toolchains/x86-4.9/prebuilt/linux-x86_64/bin/i686-linux-android-
#export SYSROOT=/home/yannik/Downloads/android-ndk-r11c/platforms/android-21/arch-arm
export CC=${CROSS}gcc
export LD=${CROSS}ld
export AS=${CROSS}as
export AR=${CROSS}ar
#make ARCH=arm build
make
