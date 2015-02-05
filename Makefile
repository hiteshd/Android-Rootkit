# Written by Hitesh Dharmdasani
# "your droid is my droid"
# ISA 674
#

obj-m += sys_call_table.o

INC_PATH=android-dev/ndk/android-ndk-r7/platforms/android-4/arch-arm/usr/include
LIB_PATH=android-dev/ndk/android-ndk-r7/platforms/android-4/arch-arm/usr/lib
GCC=android-dev/ndk/android-ndk-r7/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-gcc-4.4.3
LD_FLAGS = -Wl,-dynamic-linker,/system/bin/linker,-rpath-link=$(LIB_PATH) -L$(LIB_PATH) -nostdlib -lc -lm -lstdc++
PRE_LINK = $(LIB_PATH)/crtbegin_dynamic.o
POST_LINK = $(LIB_PATH)/crtend_android.o

CROSS_COMPILE=android-dev/ndk/android-ndk-r7/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-
KERNEL_DIR = android-dev/evervolv/android_kernel_htc_qsd8k-jellybean/
VERSION = v1.1

all:
	make -C $(KERNEL_DIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) CFLAGS_MODULE=-fno-pic modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f sys_call_table_inst

%.o:%.c
	$(GCC) -w -I$(INC_PATH) -c $< -o $@

