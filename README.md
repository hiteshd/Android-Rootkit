Android-Rootkit
===============

A rootkit for Android. Based on [Android platform based linux kernel rootkit](http://www.phrack.org/issues/68/6.html#article) from Phrack Issue 68

Part of [ISA 673](http://cs.gmu.edu/~astavrou/isa673_S10.html) a class project. Adding it here just because there is not just enough documentation out there to do this for Android

I appreciate any pull requests as long as they extend functionality and dont do harm

Kernel Build Specs
===================
* Using kernel tree from [here](https://github.com/Evervolv/android_kernel_htc_qsd8k)

* Using ROM image from [here](http://cos-bravo-ics.googlecode.com/files/COS-Bravo-Jb-alpha1.zip)

* Using Android NDK toolchain 4.4.3 from Google.

* Tried and tested on HTC Bravo running kernel version 2.6.38.8


Module Information
==================

Filename:      `sys_call_table.ko`
Desciption:    This rookit is developed to intercept the following calls
* SYS_WRITE
* SYS_READ
* SYS_CREAT
* SYS_MKDIR
* SYS_RMDIR
* SYS_KILL
* SYS_OPEN
* SYS_CLOSE
* SYS_GETDENT
* SYS_UNLINK
* SYS_KILL

Author:         Hitesh Dharmdasani <hdharmda@gmu.edu>

License:        GPL v2

Depends:        Android NDK, Kernel source tree of target

Vermagic:       2.6.38.8-cos-bravo-jellybean+ preempt mod_unload ARMv7


Other details
=============

* The source tree will not complile to give you a zImage that you should use. A hack around it was to just use a pre built rom with the same specs
* If you are facing vermagic issues. Fix them by the obvious.
  * Fix entry in utrelease.h
  * Fix entry in kernel.release
  * DO NOT 'make' the kernel source tree after you do this
* Edit the makefile to suit your paths for the NDK and the kernel source tree for your Android Operating system
* The rootkit compiles as a kernel object and needs to be run on the phone.
    * `# insmod sys_call_table.ko`
    * `# ./sys_call_table_inst`
* Use `dmesg` to debug
