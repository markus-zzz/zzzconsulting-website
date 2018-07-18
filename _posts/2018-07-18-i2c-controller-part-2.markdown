---
layout: post
title:  "A basic I2C controller - part 2"
comments: true
categories: hardware rtl verilog i2c qemu linux
---

In this post we will integrate our I2C controller simulation with QEMU. If
you didn't already know [QEMU](https://en.wikipedia.org/wiki/QEMU) is a free and
open-source emulator that performs hardware virtualization. More specifically
it will allow us to emulate an ARM system where we can connect our I2C
controller as a memory mapped device. We are going to pick the ARM
VersatileExpress development board, that is already emulated by QEMU, and
modify it by adding our device.

## Get, modify and build QEMU
```
export $ROOT=some/path
cd $ROOT
wget https://download.qemu.org/qemu-2.12.0.tar.xz
tar xf qemu-2.12.0.tar.xz
mkdir qemu-build
cd qemu-build
../qemu-2.12.0/configure --prefix=$ROOT/qemu-install
make install -j8
export PATH=$ROOT/qemu-install/bin:$PATH
```

To register our device we need to add the following line to the
*a9_daughterboard_init* function of *qemu-2.12.0/hw/arm/vexpress.c*
```
sysbus_create_simple("axi_master_client_device", 0x1e00b000, NULL);
```
For the actual implementation of our device (in our repository the file
*qemu/axi_master_client_device.c*) I simply sym-linked to it, and remembered to
update the *hw/arm/Makefile.objs* so that it gets built.

## Prepare the Linux system
Second we are going to go ahead and build a complete little Linux system
(kernel and busybox file system). While it is strictly not needed for what we
are trying to achieve in this post it will be convenient to have for testing as
you will see shortly.

Before we do anything we need to make sure that we have a usable cross compiler
for our intended target. I grabbed a pre-built one from
[here](https://releases.linaro.org/components/toolchain/binaries/latest-6/arm-linux-gnueabihf/).
Then we need to acquire build the kernel sources
```
wget https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.17.8.tar.xz
make CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm vexpress_defconfig
make CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm -j8
```
After that BusyBox
```
wget http://busybox.net/downloads/busybox-1.29.1.tar.bz2
make CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm vdefconfig
make CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm vmenuconfig (enable static link option)
make CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm vinstall
cd _install
mkdir proc sys dev etc etc/init.d
```
put the following lines in etc/init.d/rcS
```
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
/sbin/mdev -s
```
and finally create the FS image
```
find . -print0 | cpio --null -ov --format=newc   | gzip -9 > ../initramfs.cpio.gz
```
Could use status register busy bit as clock request for simulation? Meaning
that as long as we have clock request the i2c controller keeps running
regardless if we have something to recv from the socket or not.

## Testing

### Linux boot
First let us boot up the Linux system
```
qemu-system-arm -M vexpress-a9 -kernel arch/arm/boot/zImage -dtb ./arch/arm/boot/dts/vexpress-v2p-ca9.dtb -nographic -initrd initramfs.cpio.gz -append "console=ttyAMA0 ignore_loglevel log_buf_len=10M print_fatal_signals=1 LOGLEVEL=8 earlyprintk=vga,keep sched_debug root=/dev/ram rdinit=/sbin/init"
```
QEMU has a monitor that can be entered by issuing **ctrl-a c**. There is also a
shorthand for the very useful quit command by pressing **ctrl-a x**.


At this point just verify that the Linux system boots up and that you can enter
the BusyBox shell.

### The QEMU and AXI part
BusyBox has a very convenient utility command called *devmem* that allows us to
access physical memory addresses without the hassle of having to write a kernel
module at this point (or having to write a custom user space program to fiddle
with */dev/mem* for that matter sice that is exactly what *devmem* is).

The AXI slave of our I2C controller implements a few dummy registers (they can
be written and read but serve no other purpose) that we can use to verify our
system. The three registers are mapped at 0x1e00b000, 0x1e00b004 and
0x1e00b008.

First start the RTL simulation with AXI master server
```
vvp -M. -mvpi_axi_master i2c.vvp
```
Then start the QEMU system (as described above) and use *devmem* to fire away a
few writes and verify that the values read back are as expected.

```
devmem 0x1e00b000 w 0x11112222
devmem 0x1e00b004 w 0x33334444
devmem 0x1e00b008 w 0x55556666

devmem 0x1e00b004
devmem 0x1e00b000
devmem 0x1e00b008
```
Note that we already verified that the I2C controller was working reasonably
well in the preivous post so we can delay further testing of that until the
next post where we implement a proper device driver.

## Wrap up
That concludes todays post.

Did you like this post? Questions or feedback - leave a comment below!

