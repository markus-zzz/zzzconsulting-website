---
layout: post
title:  "A basic I2C controller - part 3"
comments: true
categories: hardware rtl verilog i2c qemu linux
---

In today's post we will start writing a Linux device driver for our simulated
I2C EPROM. It will be a simple character device that shows up as */dev/eprom*
and allows data to be read and written as in

```
echo "Hello EPROM!" > /dev/eprom
cat /dev/eprom
```

Since I2C is slow and we do not want to busy-wait on the status register inside
the driver (I guess we could if we kept calling *schedule()* but lets do it
properly) we need to generate a interrupt from the block once the busy status
de-asserts.

Generating asynchronous signals from the block poses a problem for our current
socket based simulation system as it is totally synchronous and everything is
initiated from the QEMU side.

To deal with the asynchronous nature of interrupts we are going to introduce a
second socket on which the VPI code can post interrupt requests. In our QEMU
device we will create a thread that blocks on a receive on this socket and once
a IRQ arrives it will acquire the *Big QEMU Lock* and pass the IRQ on to the
QEMU internals.

Now that we got that out of the way let's get started with our device driver.
We are going to implement it as an out-of-tree kernel module and hence will
build it as
```
make CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm -C $ROOT/linux M=${PWD} modules
```

It turns out that getting an interrupt mapped through Linux in modern days is a
bit more difficult than I expected. Seemingly to get this machinery to play
along nicely with us we will have to do it the real proper way using the
*platform device* framework and adding *device tree (DTS)* entries. Doing so
will frankly require a bit of read up on my part so we will postpone that to
the next post in this series. For now let's just do it the busy-wait way and
verify that everything works.
