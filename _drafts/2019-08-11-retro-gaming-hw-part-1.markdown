---
layout: post
title:  "Designing retro gaming hardware - part 1"
comments: true
categories: hardware rtl verilog game retro
---

In this post we visit my latest pet project - that is building a custom 80's
style game console. Much fun ahead :)

## Background
The main motivating factor behind this effort was seeing the game [Celeste
(classic)](https://www.lexaloffle.com/bbs/?tid=2145) running on the
[Pico-8](https://www.lexaloffle.com/pico-8.php) fantasy console. This game is
in fact the precursor of the very well received 2018's title
[Celeste](http://www.celestegame.com/) and Pico-8 is a software emulator for a
80's game console that never existed. The limited environment offered by such a
machine has made it quite popular among makers of small games as it turns out
that having limited resources really can be a good thing when it comes to
finishing a game project.

One of the many cool things with Pico-8 is that the cartridge for any game
distributed is automatically open and its contents (sprites, tiles and maps as
well as source code) can be examined by anyone, either through the builtin
Pico-8 IDE or using the python
[picotool](https://github.com/dansanderson/picotool) module.

I for one find this game to be truly awesome and what was done with the
limited resources really impresses me. As a result the goal of my project is
now to design 80's style sprite and tile based video game hardware that is
capable of playing that game and run it on a FPGA.

## Design

Lets begin by looking at the major components of the system.

### Graphics
First there is no frame buffer (remember at this time memory is expensive and
CPUs are slow). Instead all graphics are produced on the fly having dedicated
hardware shift out pixel by pixel of movable sprites and stationary background
tiles as the video beam traces along the screen.

The lowest video resolution broadly supported by modern display hardware is
640x480@60Hz. The video timing generator for such a resolution uses a 25Mhz
pixel clock which is also quite suitable to base the rest of our design on.

For a retro system 640x480 is too high resolution so we cut it down to 320x240
by ignoring the lowest bit of the vertical video coordinate (so that even and
odd scanlines will look the same) and introducing a clock enable (set by the
lowest bit of the horizontal video coordinate) to have the sprite and tile
shifters keep the same pixel value for two consecutive horizontal video pixels.

A sprite/tile in Pico-8 is 8x8 pixels and a pixel holds 4 bits of color
information. We will do the same.

A sprite shifter module is basically a shift register combined with some
triggering logic to make the shifting start only when a programmed horizontal
position is reached.

Our system has eight sprite shifters and each of these need to be loaded per
scanline before the visual region starts. As soon as a scanline starts (or
rather when the previous one reached position 320) a FSM takes care of
performing the following steps

* Scan the 64 entry sprite descriptor table in VRAM to find the first eight that
have sprites intersecting the current scanline. While doing this load the
corresponding sprite lines into the sprite shifters and set the horizontal
trigger position.
* When the active region starts load the corresponding tile line into the tile
shifter based simply on the contents of the VRAM tile table.

The sprite descriptor format is:

|Field|Bits|Description|
|----|----|-----------|
|vpos|8|Vertical start position (0-319).|
|hpos|9|Horizontal start position (0-239).|
|idx|7|Index of sprite (0-127).|
|vflip|1|Flip pixels vertically.|
|hflip|1|Flip pixels horizontally.|
|active|1|Display sprite.|

### VRAM
As is customary the graphics system has its own memory (VRAM) to which it has
exclusive access (CPU will stall if it tries to access) except for during the
vertical blanking period when the CPU gets to update sprite descriptors and
tile tables.

#### Rationale behind dimension and memory map

Each sprite/tile is 8x8 pixel with a color depth of 4 bits giving it a
storage requirement of 32 bytes. We need at least 128 of these in VRAM so
that results in 4096 bytes.

Each sprite descriptor is 32 bits wide and we need 64 of those, i.e. 256
bytes.

With a screen resolution of 320x200 and sprite/tile size of 8x8 a total of
40x30 tiles are needed to cover the screen using a 8 bit index for each results
in 1200 bytes. Since we want to be able to index this memory without performing
multiplication the dimension is rounded up to the nearest power-of-two i.e.
64x32 resulting in 2048 bytes (in fact the size can be much less depending on
if this is row major or column major).

These extra rows and columns may come in handy whey trying to support
smooth scrolling.

All in all it is reasonable to dimension the VRAM to 8192 bytes. To allow
efficient 32 bit access from both GFX and CPU this is laid out as 2048 x
32 meaning that we need an address bus width of 11 bits.

So to sum it up VRAM is laid out as follows:

|Base|Size|Description|
|----|----|-----------|
|0|4096|Table of 128 8x8 sprite/tile bitmaps with 4-bit color depth (each bitmap consumes 32 bytes).|
|4096|256|Table of 64 sprite descriptors each of size 32 bits.|
|4352|2048|40x30 table of background tile indices (addressed as if dimensions were 64x32).|

### CPU
One thing that is not so awesome about 80's systems is the software tools
available (especially compilers), so instead of using a CPU from the era this
project will use a modern [RISC-V](https://riscv.org/) 32-bit CPU which is well
supported by modern tool-chains such as LLVM and GCC. The particular RISC-V
implementation of choice is
[PicoRV32](https://github.com/cliffordwolf/picorv32) as we favor small size
over computational efficiency.


### System memory map

|Base|Size|Memory|
|----|----|-----------|
|0x00000000|16KB|ROM|
|0x10000000|??|RAM|
|0x20000000|8192|VRAM|
|0x30000000|4|System status register|


## Current implementation

The implementation can be found on
[github](https://github.com/markus-zzz/retrocon/tree/dev). While being very
much work in progress it is currently capable of bouncing sprites around over
static background tiles as can be seen in this animation

![Bouncing sprites](/download/retrocon-sprite-bounce.apng)

The PNGs for the animation above were produced by a
[Verilator](https://www.veripool.org/wiki/verilator) based simulation using the
custom tool
[vgamon](https://github.com/markus-zzz/retrocon/blob/dev/sim/verilator/vgamon.cpp)
that captures the VGA output of the design either displaying it in real time
(i.e. on my machine ~8 frames per second) and optionally storing frames to PNG
files.

Now Verilator is pretty awesome and this tool has already proven extremely
useful. Considering the sheer amount of simulation cycles that go into
producing a single frame makes you quickly realize that traditional simulators
such as [Icarus Verilog](http://iverilog.icarus.com/) don't quite cut it for
this purpose.
```
Usage: ./vgamon [OPTIONS]

  --scale=N             -- set pixel scaling
  --frame-rate=N        -- try to produce a new frame every N ms
  --save-frame-from=N   -- dump frames to .png starting from frame #N
  --save-frame-to=N=N   -- dump frames to .png ending with frame #N
  --save-frame-prefix=S -- prefix dump frame files with S
  --exit-after-frame=N  -- exit after frame #N
  --trace               -- create dump.vcd
```
The design has been tested on a [ULX3S](https://radiona.org/ulx3s/) FPGA board
using the open source [SymbiFlow](https://symbiflow.github.io/) tools (Yosys
and nextpnr). Doing so gives a device utilization as follows for the boards
[ECP5 12F](https://www.latticesemi.com/Products/FPGAandCPLD/ECP5).
```
Info: Device utilisation:
Info:          TRELLIS_SLICE:  2822/12144    23%
Info:             TRELLIS_IO:    10/  196     5%
Info:                   DCCA:     3/   56     5%
Info:                 DP16KD:    16/   56    28%
Info:             MULT18X18D:     0/   28     0%
Info:                 ALU54B:     0/   14     0%
Info:                EHXPLLL:     1/    2    50%
Info:                EXTREFB:     0/    1     0%
Info:                   DCUA:     0/    1     0%
Info:              PCSCLKDIV:     0/    2     0%
Info:                IOLOGIC:     0/  128     0%
Info:               SIOLOGIC:     8/   68    11%
Info:                    GSR:     0/    1     0%
Info:                  JTAGG:     0/    1     0%
Info:                   OSCG:     0/    1     0%
Info:                  SEDGA:     0/    1     0%
Info:                    DTR:     0/    1     0%
Info:                USRMCLK:     0/    1     0%
Info:                CLKDIVF:     0/    4     0%
Info:              ECLKSYNCB:     0/    8     0%
Info:                DLLDELD:     0/    8     0%
Info:                 DDRDLL:     0/    4     0%
Info:                DQSBUFM:     0/    8     0%
Info:        TRELLIS_ECLKBUF:     0/    8     0%
```
The only video output on ULX3S is a HDMI compatible connector so we borrow the
code from [Project Trellis DVI](https://github.com/daveshah1/prjtrellis-dvi) to
output DVI over that connector. Works surprisingly well!

## Next steps
Some ideas for the future:
* Implement UART based means to upload CPU code and RAM contents to running
  board (having to re-synthesize as is the case now really slows down the
  development cycle).
* Implement horizontal flip and vertical flip sprite flags.
* Support for smooth scrolling background tiles (preferably in all directions).
* Look into supporting Pico-8 style sound effects.
* Fix a million bugs :)

That is it for now. Hopefully I will make some more progress soon!
