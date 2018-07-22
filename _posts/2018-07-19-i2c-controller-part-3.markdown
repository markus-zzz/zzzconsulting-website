---
layout: post
title:  "A basic I2C controller - part 3"
comments: true
categories: hardware rtl verilog i2c qemu linux
---

In today's post we will start writing a Linux device driver for our simulated
I2C EPROM. It will be a simple character device that shows up as */dev/zzz-i2c-eprom*
and allows data to be read and written as in

```
echo "Hello EPROM!" > /dev/zzz-i2c-eprom
cat /dev/zzz-i2c-eprom
```
## Interrupts
Since I2C is slow and we do not want to busy-wait on the status register inside
the driver (we actually could if we kept calling *schedule()* but lets try to
do it properly) we need to generate a interrupt from the block once the busy
status de-asserts.

Generating asynchronous signals from the block poses a problem for our current
socket based simulation system as it is totally synchronous and everything is
initiated from the QEMU side.

To deal with the asynchronous nature of interrupts we are going to introduce a
second socket on which the VPI code can post interrupt requests. In our QEMU
device we will create a thread that blocks on a receive on this socket and once
a IRQ arrives it will acquire the *Big QEMU Lock* and pass the IRQ on to the
QEMU internals.

## Problems
Unfortunately it turns out that getting an interrupt mapped through Linux in
modern days is a bit more difficult than I expected. Seemingly to get this
machinery to play along nicely with us we will have to do it the real proper
way using the *platform device* framework and adding *device tree (DTS)*
entries. Doing so will quite frankly require a bit of read up from my side so
we will postpone that to the next post in this series. For now let's just do it
the busy-wait way and verify that everything works.

## Implementation
Now that we got that out of the way let's get started with our device driver.
We are going to implement it as an out-of-tree kernel module and hence will
build it as
```
cd $ZZZ_ROOT/linux
make CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm -C $ZZZ_ROOT/linux M=${PWD} modules
```


The code for the non-interrupt driven device driver can be found in
[linux/i2c-eprom-driver.c](https://github.com/markus-zzz/i2c-controller/blob/master/linux/i2c-eprom-driver.c).
Notice that we are reusing a lot of code from
[axi_master_client.c](https://github.com/markus-zzz/i2c-controller/blob/master/axi_master_client.c)
from the first post.

## Testing
Once built and moved to the BusyBox filesystem the module installs as follows
into our running Linux system
```
insmod i2c-eprom-driver.ko
```
Since we already setup the BusyBox hotplug daemon *mdev* in the previous post
the device file */dev/zzz-i2c-eprom* should automatically appear in the file
system once the module is installed.

Now we can finally try out the full system! Let's start by clearing the memory
with zeros, then write some string into it and verify that it reads back.
```
dd if=/dev/zero of=/dev/zzz-i2c-eprom count=16 bs=1
echo "Hello world!!!" > /dev/zzz-i2c-eprom
cat /dev/zzz-i2c-eprom
```
