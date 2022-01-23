---
layout: post
title:  "[DRAFT] Making some PCBs - part 4 (ECP5)"
math: true
mermaid: true
comments: true
categories: hardware
---

# Stage 3 - ECP5
4-layer board (signal, power, gnd, signal).

Study these designs:
* <https://github.com/mattvenn/basic-ecp5-pcb> - most basic ECP5 design, highly interesting.
* <https://github.com/emard/ulx3s> - ULX3S design

The
[LFE5U-12F-6BG256C](https://www.mouser.se/ProductDetail/Lattice/LFE5U-12F-6BG256C?qs=w%2Fv1CP2dgqpgiPWLwc1Bzg==)
does indeed seem like the most suitable model and package.

Although routing a 256 pin BGA does seem intimidating one has to keep in mind
that a large number of these balls will be *no-connect*. Since this is a custom
board for a given application we only need to route what we use. This situation
is substantially different compared to doing a generic development board where
you really want to expose as many pins and interfaces as possible.

Another simplifying matter is that this is a FPGA so it has very few fixed
function pins and interfaces (expect the fixed function ones like JTAG and
flash SPI) can generally be brought out in way that will cause no trace
crossing on the PCB.

So besides power and ground how many signal do we need expose from the FPGA?

| Interface   | Pins |
| ----------- | -----|
| HyperRAM    |  12  |
| HDMI        |  10  |
| JTAG        |   5  |
| Flash       |   5  |
| MAX9850     |   5  |
| USB         |   4  |

Okay so it is quite a few but still much less than the 197 I/Os that are
available in the given package.

## Configuration

When first looking into how the device is to be configured while on the PCB the
options and tools involved seem quite a bit overwhelming. However after doing
some googling and reading up a bit I think the following answers most of my
worries (mostly by confirming that this process does indeed depend on well
established and standardized techniques and not so much proprietary magic).

* Use JTAG (TCK,TMS,TDI and TDO) as main means of configuration. Either driving
it with external JTAG-cable, an integrated FTDI chip or onboard MCU. Timing is
not critical, bit-banging is fine, the TCK may stop etc.

* For non-volatile bitstream storage use SPI connected flash. ECP5 acts as SPI
master. The FPGAs JTAG interface allows direct programming of the SPI connected
flash memory.

* The Symbiflow tools support generating both .bit files and .svf files. The
former contains the configuration bitstream only while the latter contains JTAG
commands to setup programming and the entire contents of the bitstream. This
allows for programming with a JTAG tool that does not know anything about the
actual device (i.e. all instructions are in the .svf file).

* There are several tools for programming the ECP5. ujprog, fleajtag, OpenOCD and
even one in python <https://github.com/emard/esp32ecp5>. The last one does
bit-banging from a MCU while the first three relay on a USB connected FTDI chip
for JTAG. OpenOCD only support programming with .svf files while ujprog and
fleajtag can do .bit directly. That is they contain code to write the
appropriate JTAG registers to initiate the programming. Actually IEEE 1532
defines a standardized JTAG based interface for programming that the ECP5
follows. So ujprog has code to interface the FTDI chip as well as code to issue
the relevant IEEE 1532 commands over the JTAG interface.

* Probably don’t need to care much about the PROGN, INIT and DONE signals.

* The interface between the ECP5 and FTDI need only be the 4 wire JTAG signals.
Often the FTDI also provides usb-serial port services but this is not strictly
needed. Actually the FTDI chips are really expensive (same cost as the ECP5
12F) so while it may be useful for the development version of the board it will
not go on any production variant.

In fact since FTDI appears to be a generally unlikable company it would be a
good thing to try to not use their products at all. So instead of putting an
overpriced FTDI chip on each board let us use an external JTAG cable for
programming and only put a JTAG pin-header on the PCB (with pull-down on TCK
but nothing else). Once the board has been "factory" programmed via JTAG
subsequent updates could be handled via the USB interface.  During FPGA power
up the configuration bitstream is read from the SPI flash but after that the
SPI interface pins are controllable by the FPGA fabric (according to
documentation). So in theory once our FPGA core is up and running we can
receive a new configuration bitstream via our USB interface and then write that
to the SPI flash so that it will be used during the following FPGA power up
sequence.

## PCB manufacturing

The chosen ECP5 BGA package is 0.8mm pitch so in order to escape route the
inner rings (mostly power and gnd) rather small vias need to be placed (via
diameter needs to be less than 0.55mm). Unfortunately it turns out that making
such small vias exceeds the capabilities of Aisler so a different PCB service
needs to be used. These vias are just within the capabilites of the Oshpark so
we have to turn to them instead.

When it comes to PCB services the order of preference for me would be Aisler
(Europe), Oshpark (US), JLCPCB and PCBWAY (China).

## PCB layout

Thinking that we can mount the switch mode voltage regulators and support
circuitry on the back side of the board. Also if possible decoupling capacitors
for the BGA can go on the back side as well.
