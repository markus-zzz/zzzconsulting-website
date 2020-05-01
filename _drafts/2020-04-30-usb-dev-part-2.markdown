---
layout: post
title:  "USB device part-2"
math: true
comments: true
categories: hardware
---

In this post we pick up where we left of last time and start looking at the
design and implementation of the USB device that I have been working on. First
things first though, the code for the entire project can be found in [this
github repository](https://github.com/markus-zzz/usbdev/tree/dev). The reader
is advised to have that readily available.

The first major goal of this series will be to have the device play along
during the USB enumeration process so that the host can set address and read
relevant descriptors. This can be verified by making sure that the device shows
up properly when issuing a  `lsusb` on my Linux workstation.

## Design of usbdev (and SoC)

### Clock recovery

The signaling in USB consists of the differential pair D+ and D-. For a *full
speed (FS)* device the bit rate is 12Mbit/s so if we were able to sample at
exactly the right spot a 12Mhz clock should suffice, in reality though this is
the tricky bit. To aid in clock recovery USB employs both NRZI encoding and
bit-stuffing to ensure that the differential pair will contain a level
transition at least every seven bit times.

With this in mind it would seem a reasonable approach to run the design at
48Mhz and oversample the differential pair with a factor of four. More
precisely by obtaining four equally spaced samples for each bit time we should
be able to adjust the real sampling position (in terms of 1/4 bit times) to be
as far away for any level transition as possible (i.e. in the middle of the eye
diagram).

So running at a 48Mhz clock we have a 2-bit counter (`reg [1:0] cntr`)
incrementing each cycle (except for when adjusting). When the counter equals
zero we perform the real sample. For every 48Mhz cycle we also sample and shift
the value into a four bit shift register (`reg [3:0] past`). Since we want any
possible level transition to occur in the middle of this shift register we
either advance or delay the counter with one increment depending on if a
transition occurred early or late in the shift register.
```
  // A bit transition should ideally occur between past[2] and past[1]. If it
  // occurs elsewhere we are either sampling too early or too late.
  assign advance = past[3] ^ past[2];
  assign delay   = past[1] ^ past[0];
```
If `advance` is active the counter increments by two and if `delay` is active
there is no increment (for the given 48Mhz cycle).

After seeing some transitions this should be able to adjust the sampling point
to where the signal lines are stable.

This is not the only option for clock recovery though but it is the simplest
one to implement. However if the bit rate was significantly higher compared to
the frequency our design is clocked at (e.g. USB *high-speed* at 480Mbit/s)
other methods would have to be used. In such cases I would suspect that
sampling at the exact bit rate and then phase adjusting the clock (with help of
a PLL) until the synchronization pattern is reliably detected would be the
method of choice. Of course simply phase adjusting until the synchronization
pattern is detected would not be enough, you probably want a FSM to find the
midpoint of highest and lowest phase adjustment that makes the pattern
detectable.

A variant of the previous approach would be to use adjustable delay lines on
the inputs (that some FPGAs support) instead of phase adjusting the clock.

### The HW/SW interface

To control the USB block some kind of hardware/software interface needs to be
created. I have chosen the simplest possible design that came to mind.

Each endpoint has a 8 byte buffer in RAM, starting at RAM address zero comes
the 16 IN endpoints immediately followed by the 16 OUT endpoints. In total 256
bytes of RAM are used for endpoint buffers.

In addition the following registers are exposed to the CPU.

 Register           | R/W | Address
--------------------|-----|-----------
R_USB_ADDR          | R/W | 0x20000000
R_USB_ENDP_OWNER    | R/W | 0x20000004
R_USB_CTRL          |   W | 0x20000008
R_USB_IN_SIZE_0_7   | R/W | 0x2000000C
R_USB_IN_SIZE_8_15  | R/W | 0x20000010
R_USB_DATA_TOGGLE   | R/W | 0x20000014
R_USB_OUT_SIZE_0_7  | R   | 0x20000018
R_USB_OUT_SIZE_8_15 | R   | 0x2000001C

I havn't bothered documenting them in detail yet but essentially it is as follows.

* *R_USB_ADDR* - is the 7-bit device address.
* *R_USB_ENDP_OWNER* - The 16 low bits correspond to the 16 IN endpoints and the 16 high bits correspond to the 16 OUT endpoints. A set bit indicates that the corresponding endpoint buffer is handed over to the USB block. This means that the corresponding endpoint will accept one IN/OUT packet (with data) and ACK after which the bit will be cleared and the buffer is handed over to the CPU. When the CPU owns a buffer the USB block will respond with NAK to all IN/OUT+DATA packets.
* *R_USB_CTRL* - Control pullups for attach.
* *R_USB_IN_SIZE_0_7* - 4-bits per endpoint indicate the number of bytes in the corresponding buffer.
* *R_USB_IN_SIZE_8_15* - 4-bits per endpoint indicate the number of bytes in the corresponding buffer.
* *R_USB_DATA_TOGGLE* - The 16 low bits select the data toggle (DATA0/DATA1) to be sent for the 16 IN endpoints and the 16 high bits select the data toggle to expect for the 16 OUT endpoints.
* *R_USB_OUT_SIZE_0_7* - 4-bits per endpoint indicate the number of bytes in the corresponding buffer.
* *R_USB_OUT_SIZE_8_15* - 4-bits per endpoint indicate the number of bytes in the corresponding buffer.

Of course a primitive interface like this requires a fair amount of CPU
intervention and if one wants to offload the CPU and achieve higher performance
quite a lot of this handling could probably be automated by the USB block
itself.

### SoC

The resulting SoC consists of the USB device block, a PicoRV32 RISCV CPU, RAM
and ROM. From the CPU's side the memory map is as follows.

|Base|Size|Memory|
|----|----|-----------|
|0x00000000|16KB|ROM|
|0x10000000|4KB|RAM|
|0x20000000|32B|USB control & status registers|

The USB device block's endpoint buffers reside in RAM and the block has
priority over the CPU when accessing. RAM is constructed of four 8-bit banks.
The CPU is connected by a 32-bit bus while the USB block has a 8-bit bus. As
mentioned USB has priority when accessing RAM but in practice this should
result in minimal stall cycles for the CPU as its clock is significantly higher
than the rate of which USB will read/write bytes.

## Simulation

The simulation environment is based around Verilator and a set of C++ classes
to build, manipulate and dissect USB packets.

### usb-pack-gen

The USB packet generation and manipulation code is found in
[sim/usb-pack-gen.h](https://github.com/markus-zzz/usbdev/blob/dev/sim/usb-pack-gen.h)
and
[sim/usb-pack-gen.cpp](https://github.com/markus-zzz/usbdev/blob/dev/sim/usb-pack-gen.cpp). It allows both encoding and decoding of USB packets. In essence it consists of three layers

* UsbPacket - Base class for the various USB packet types (SETUP,IN,OUT,DATA0,DATA1,ACK,NAK) that allow easy high level manipulation. The UsbPacket.
* USbBitVector - A sequence of bits.
* UsbSymbolVector - A sequence of USB symbols (J,K,SE0,SE1).

There are methods for translating in both directions performing the necessary
steps such as CRC calculation, bit-stuffing and NRZI encoding.

### Test-suite

The test-suite is invoked by the `sim/runner.pl` script that will execute all
tests found in `sim/tests` (or the ones given as argument). Each
`sim/tests/test_XXX.c` consists of both firmware code to be compiled for the
RISCV CPU as well as usb-pack-gen code compiled into the Verilator based
simulation environment.

To get a feel for what a test looks like I suggest studying
[sim/tests/test_003.c](https://github.com/markus-zzz/usbdev/blob/dev/sim/tests/test_003.c).
It is probably a good idea to start with running the test and looking at the
decoded USB traffic.
```
./runner.pl tests/test_003.c
sigrok-cli -i test_003.sim.sr -P usb_signalling:dp=0:dm=1,usb_packet | awk '/usb_packet-1: [^:]+$/{ print $0 }'
```
Should give us something like this (but without the host/device annotations I
added manually).
```
host   : OUT ADDR 0 EP 0
host   : DATA0 [ 23 64 54 AF CA FE ]
device : ACK
host   : IN ADDR 0 EP 1
device : NAK
host   : IN ADDR 0 EP 1
device : DATA0 [ 24 65 55 B0 CB FF ]
host   : ACK
host   : IN ADDR 0 EP 1
device : NAK
```
So the host sends six bytes of data to endpoint zero and the device
acknowledges it. The host tries to read from endpoint one but the device is
busy (the dummy loop reading the address register) so responds with a not
acknowledge. The host tries to read again and this time the device responds
with the byte array with each element incremented by one. The host acknowledges
the received data. The host tries to read yet again but now the endpoint buffer
has been handed back to the CPU so the USB block responds with not acknowledge.

Now with this in mind it should be rather clear what is going on in the test
case.

### Test-suite artifacts

When run the test-suite outputs several useful artifacts. These are

* test_XXX.comp.log -
* test_XXX.sim.log -
* test_XXX.sim.vcd -
* test_XXX.sim.sr -

## A real world example

I thought we finish this post by using some hard coded driver code to perform
the first few steps (up to changing address) of the USB enumeration process
with the ULX3S connected to my Linux workstation.

## Next steps

At this point we should have hardware support to start implementing the driver
code running on the CPU. We will surely discover more RTL bugs along the way
but we try to fix them when they show.

That is it for today. As always if you have questions or feedback - leave a
comment below!
