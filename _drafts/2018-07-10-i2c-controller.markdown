---
layout: post
title:  "Implementing a basic I2C controller"
date:   2018-07-09 22:00:50 +0200
comments: false
categories: hardware rtl verilog
---

Write some stuff about the implementation and simulation (vpi socket soft axi master)

Write the following posts
- Implementation
- Simulation
- Linux driver

For a recent project I needed to use the audio codec on a ZedBoard to generate
a sine wave. The codec had quite an extensive set of registers that needed to
be configured before it was willing to output any sound at all and these
registers were of course accessible to the FPGA via the I2C two wire bus
protocol. Now I2C isn't all that complicated, and the register writes were only
needed at startup, so you could get away with a simple software implementation
using GPIOs. However I thought it would be neater to do an actual block for it
that connected to the AXI bus.

The simple AXI interface should expose these registers

#### Control register

|Field|Bits|Description|
|-----|----|-----------|
|we|1|Write enable i.e. should we be driving the SDA during the transaction|
|start|1|Generate start condition before|
|stop|1|Generate stop condition after|
|byte|8|Data byte to write (could be address in which case the 8:th bit indicates read or write during following transaction)|

#### Status (and read) register

|Field|Bits|Description|
|-----|----|-----------|
|busy|1|Controller busy with ongoing transaction|
|ack|1|Acknowledge status for last transaction|
|byte|8|Data byte read if last transaction was read (we always sample data even if we are the ones driving SDA ourselves)|

For testing I found this
[this](https://github.com/olofk/i2c/blob/master/bench/verilog/i2c_slave_model.v)
slave model of a I2C EPROM.

Now since the interface is AXI writing a testbench for this in verilog seemed
like a pain and it seemed much easier if we could generate the AXI register
writes (and reads) from plain C code. Luckily it is rather easy to interface
infterface a Verilog simulator with your favorite programming language (C of
course) by using VPI (Verilog Programming Interface)

Using VPI allows us connect a callback to signal events (say posedge of clk)
and from there you can inspect and modify any other signal.  So we connect our
callback to the negative edge of the AXI bus clock and from there we manipulate
the bus signals with a simple software driven state machine. To make this a bit
more usable we have the simulator callback listen on a socket for read/write
commands.

Now put all that is needed to run the C test bench writing and reading from the
EPROM on github (with copyright info) and provide instructions for running it.

- once that works it concludes this post
- next post is about connecting the simulation to QEMU
- post about booting linux on QEMU and writing device driver

