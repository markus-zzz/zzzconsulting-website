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
