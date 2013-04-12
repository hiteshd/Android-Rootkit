Android-Rootkit
===============

A rootkit for Android. Based on "Android platform based linux kernel rootkit" from Phrack Issue 68

Part of a class project. Adding it here just because there is not just enough documentation out there to do this for android

Kernel Build Specs
===================
* Using kernel tree from https://github.com/Evervolv/android_kernel_htc_qsd8k

* Using ROM image from http://cos-bravo-ics.googlecode.com/files/COS-Bravo-Jb-alpha1.zip

* Using Android NDK toolchain 4.4.3 from Google.

* Tried and tested on HTC Bravo running kernel version 2.6.38.8


Module Information
==================

filename:       sys_call_table.ko
description:    This rookit is developed to intercept the following calls
   					 SYS_WRITE, SYS_READ ,SYS_CREAT ,SYS_MKDIR ,SYS_RMDIR ,SYS_KILL ,SYS_OPEN ,SYS_CLOSE ,
 						 SYS_GETDENT ,SYS_UNLINK, SYS_KILL 
 						 						
author:         Hitesh Dharmdasani <hdharmda@gmu.edu>
license:        GPL
depends:        
vermagic:       2.6.38.8-cos-bravo-jellybean+ preempt mod_unload ARMv7 


Other details
=============

* The source tree will not complile to give you a zImage that you should use. A hack around it was to just use a pre built rom with the same specs
* If you are facing vermagic issues. Fix them by the obvious.
  -- Fix entry in utrelease.h
  -- Fix entry in kernel.release
  -- DO NOT 'make' the kernel source tree after you do this
