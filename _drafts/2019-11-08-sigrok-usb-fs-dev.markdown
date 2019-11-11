---
layout: post
title:  "Sigrok and USB FS device"
math: true
comments: true
categories: hardware
---

A while ago a bought a [DSLogic
Plus](https://www.dreamsourcelab.com/product/dslogic-plus/) USB-based Logic
Analyzer that I never got around to try. That is  until a few days ago. The
thing comes with its own analyzer software running on the PC called
[DSView](https://www.dreamsourcelab.com/download/), but I never bothered trying
that and instead went with the better known [sigrok](https://sigrok.org/) (of
which DSView is a derivative).


## Setting up sigrok for the DSLogic Plus

Building from source seemed rather complicated with many dependencies needing
to be met but luckily they also provide self contained
[AppImage](https://appimage.org/) binaries for download
[here](https://sigrok.org/wiki/Downloads). So I picked up a PulseView and a
sigrok-cli binary.

Although being self contained two additional steps were required to get things
going. 

First setting up udev rules
```
git clone git://sigrok.org/libsigrok
cp libsigrok/contrib/*.rules /etc/udev/rules.d/
```
and second, due to legal reasons, the self contained sigrok binaries do not
contain the necessary firmware for the DSLogic Plus so we need to grab a script
that fetches those separately
```
git clone git://sigrok.org/sigrok-util
sudo sigrok-util/firmware/dreamsourcelab-dslogic/sigrok-fwextract-dreamsourcelab-dslogic
```

## Test capture

Attaching two the to the USB **D+** and **D-** of a STM32 based thumb device and
capturing the signaling that occurs when the device is connected to the bus.

For background on USB 2.0 the spec summary [USB in a
NutShell](https://www.beyondlogic.org/usbnutshell/usb1.shtml) is a highly
recommended read. The full specification is available
[here](https://www.usb.org/document-library/usb-20-specification).

Insert picture here.

Sigrok provides a very neat protocol decoder for USB (as well as for many other
protocols).

Insert screen dump here.

It is interesting to observe what happens when a new device is connected

```
# Newly connected device has address 0.
host   : SETUP ADDR 0 EP 0
host   : DATA0 [ 80 06 00 01 00 00 40 00 ]
device : ACK
host   : IN ADDR 0 EP 0
device : NAK
host   : IN ADDR 0 EP 0
device : DATA1 [ 12 01 00 02 02 02 00 40 83 04 40 57 00 02 01 02 03 01 ]
host   : ACK

host   : OUT ADDR 0 EP 0
host   : DATA1 [ ]
device : NAK
host   : OUT ADDR 0 EP 0
host   : DATA1 [ ]
device : ACK

host: <RESET for 50ms>

# SET_ADDRESS (=0x05) to 10 (=0x0A)
host   : SETUP ADDR 0 EP 0
host   : DATA0 [ 00 05 0A 00 00 00 00 00 ]
device : ACK
host   : IN ADDR 0 EP 0
device : NAK
host   : IN ADDR 0 EP 0
device : DATA1 [ ]
device : ACK

# From this point on the device has address 10.

host   : SETUP ADDR 10 EP 0
host   : DATA0 [ 80 06 00 01 00 00 12 00 ]
device : ACK
...
```
what appears to happen here is that the host begins by reading the *device
descriptor* containing, among other things, the highest USB version this device
supports, *idVendor* and *idProduct*. If these are satisfactory(?) the host
goes ahead and drives a long reset followed by assigning the device a new
address.

A simple `lsusb` shows that *idVendor* and *idProduct* appear in the response
message retrieved before the reset.
```
Bus 001 Device 010: ID 0483:5740 STMicroelectronics STM32F407
```
Actually I do not know if the host

## Decoding USB

If you are so inclined you might now wonder what kind of digital circuitry it
would take to decode the USB signaling in say an FPGA. In this case a
reasonable first goal to set might be to be able to decode the *Start Of Frame
(SOF)* packet that goes out on the bus every 1ms.

Using the LA to capture some SOF packets and then feed them into a verilog
simulation is in fact not very difficult. Keep in mind though that sampling
rate here is 50Mhz and not 48Mhz so there may be some syncronization issues.
```
sigrok-cli --input-file sofs.sr -O csv | awk 'BEGIN{FS=","}{print $2$3}' - > sofs.vh
```

### ULX3S setup

Connected both to differential IO cells and single ended IO cells. Since we
need to both drive and sample differential as well as single ended. Still weird
though as ECP5 docs kind of suggest that all of that could be done within one
IO cell (pair).

Insert picture of schematic also mention controllable pull-up resistors.

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
