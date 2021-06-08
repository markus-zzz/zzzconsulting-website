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

* <https://github.com/blackmesalabs/hyperram> - slow but portable controller
* <https://github.com/gregdavill/litex-hyperram> - full performance controller for ECP5 in Migen
* <https://1bitsquared.com/products/pmod-hyperram>
* <https://www.youtube.com/watch?v=dThZDl-QG5s>

5x5 BGA means that the two outermost rings can be routed in the same layer
(only one ball remaining in the middle but that can also escape in the same
layer as one corner is missing a ball).

### Stage 2 - MAX9850
2-layer board.

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
