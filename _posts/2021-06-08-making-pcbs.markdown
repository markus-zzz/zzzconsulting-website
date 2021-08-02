---
layout: post
title:  "[DRAFT] Making some PCBs - part 1"
math: true
mermaid: true
comments: true
categories: hardware
---

**NOTE: This post is not finished yet but instead of keeping it as a hidden
draft I have decided to publish it anyway - mostly to serve as a notebook
to myself while I work on the project.**

## Introduction

For a while now I have had a desire to make a custom PCB for the MyC64 project,
a board with suitable display interface and perhaps some tiny keyboard.
Something that could be used in a handheld. A handheld FPGA based C64 emulator
now that would be pretty cool.

Back in 2019 I did play a bit with [KiCad](https://www.kicad.org/) making a
very simple (through hole) PCB for [PMOD like VGA
interface](https://github.com/markus-zzz/myvgapmod) but wrt complexity that is
light years away from what I need to do now.

A board for the MyC64 will need to include the ECP5 FPGA with its, at least,
256 ball BGA packaging as well as HyperRAM 24 ball BGA and multiple other fine
pitch surface mount components.

While I certainly need to learn a lot about board design and KiCad that bit
does not scare me half as much as doing the actual assembly for those parts.

As a side not I spent some time during summer vacation to build a workbench for
my lab corner where I can put all my equipment. This is how it turned out.

![Workbench](/download/pcb/workbench.jpg)

## The Plan

So to get started I thought I could begin with making a few rather simple PMOD
boards for some of the components that I intend to use on the final board. This
time actually following [the Pmod
specification](https://www.digilentinc.com/Pmods/Digilent-Pmod_%20Interface_Specification.pdf).
This way I get a chance to improve my limited KiCad and assembly skills along
the way as well as trying out interfacing the components with the FPGA on the
trusted ULX3S board. That seems like a sound plan.

### Stage 1 - HyperRAM
2-layer board.

I have 10 of these
[S27KL0641DABHI020](https://www.mouser.se/ProductDetail/cypress-semiconductor/s27kl0641dabhi020/?qs=74EMXstkWMUC1hAb%252bS%2F79g==&countrycode=DE&currencycode=EUR)
laying around.

5x5 BGA means that the two outermost rings can be routed in the same layer
(only one ball remaining in the middle but that can also escape in the same
layer as one corner is missing a ball).

I finally had a go at this and put together quick and dirty PCB in KiCAD and
had <https://aisler.net/> manufacture it for me. By quick and dirty I mean that
the layout is ugly and the traces are not length matched so I would not expect
to get any performance out of it. The board should be functionally correct
though but this has not yet been verified.

Two different soldering methods were evaluated and I tried capturing the chip
profile with a cheap cell-phone camera through a binocular microscope.

On the left only tacky flux was applied and then using hot air from above
(station set to 300C at 50l/min) the chip was soldered in place. After a while
the chip noticeably sunk into place and I tried to nudge it several times with
a pair of tweezers after which it immediately bounced back. This was a quick
method but the end result is that the chip sits flush with the board and visual
inspection is not really possible.

On the right no flux was used but instead solder paste was applied through a
stencil. The board was placed on a hot-plate that slowly rose to a temperature
set to 220C after which hot air was applied from above (with station set to
300C at 40l/min). It was not as noticeable when the chip sunk into place and I
did not dare to nudge it (as it was still on the hot-plate). The end result
looks much nicer though with the solder balls clearly visible and one can
clearly see that there is no bridging.

![HyperRAM BGA](/download/pcb/hyperram-bga-pcb.jpg)

Eventually I will try to verify the boards with one of the controllers from the
list below and report the results.

* <https://github.com/blackmesalabs/hyperram> - slow but portable controller
* <https://github.com/gregdavill/litex-hyperram> - full performance controller for ECP5 in Migen
* <https://1bitsquared.com/products/pmod-hyperram>
* <https://www.youtube.com/watch?v=dThZDl-QG5s>


### Stage 2 - MAX9850
2-layer board.

Still have not decided if I will do an audio board but this seems like a nice
DAC.

* <https://www.maximintegrated.com/en/products/analog/audio/MAX9850.html>

### Stage 3 - ECP5
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

#### Configuration

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


