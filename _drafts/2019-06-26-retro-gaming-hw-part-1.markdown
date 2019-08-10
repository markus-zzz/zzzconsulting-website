---
layout: post
title:  "Designing retro gaming hardware - part 1"
comments: true
categories: hardware rtl verilog game retro
---

In this post we visit my latest pet project - designing a custom 80's style
game console. 

One of the motivating factors behind this effort was seeing that the
precursor/proof-of-concept for the very well received 2018's game
[Celeste](http://www.celestegame.com/) was developed on the
[Pico-8](https://www.lexaloffle.com/pico-8.php) fantasy console. Pico-8 is a
virtual implementation of a 8-bit 80's game console that never existed. It has
become quite popular and has a community for sharing game cartridges whose
contents can be fully examined either inside the Pico-8 IDE or using the python
[picotool](https://github.com/dansanderson/picotool) module.

The Pico-8 version of Celeste can be found
[here](https://www.lexaloffle.com/bbs/?tid=2145) and even played from inside
the browser.

I for one find this game to be truely awesome and what was done with the
limited resources really impresses me. As a result the goal of this project is
to design 80's style sprite and tile based video game hardware that is capable
of playing that game.

Design

CPU
One thing that is not so awesome about 80's systems is the software tools
available (especially compilers), so instead of using a CPU from the era this
project will use a modern RISC-V 32-bit CPU which is well supported by modern
toolchains such as LLVM and GCC.

Graphics
The lowest video resolution broadly supported by modern display hardware is
640x480@60Hz. The video timing generator for such a resolution uses a 25Mhz
pixel clock which is also quite suitable to base the rest of our design on.



Sprites

Tiles

Memory Map

