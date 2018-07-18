---
layout: post
title:  "A basic I2C controller - part 1"
comments: true
categories: hardware rtl verilog i2c
---

For a recent project I needed to interface the audio codec on a ZedBoard to
generate a sine wave. The codec had quite an extensive set of registers that
needed to be configured before it was willing to output any sound at all and
these registers were of course accessible to the FPGA via the I2C two wire bus
protocol. Now I2C isn't all that complicated, and the register writes were only
needed at startup, so you could get away with a simple software implementation
using GPIOs. However I thought it would be neater to implement an actual
controller block in the FPGA and connect that to the AXI bus.

After some thinking I decided that the AXI interface of my simple I2C
controller should expose these two registers:

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
|byte|8|Data byte read if last transaction was read (we always sample data even if we are driving SDA ourselves)|

For testing I found this
[this](https://github.com/olofk/i2c/blob/master/bench/verilog/i2c_slave_model.v)
slave model of a I2C EPROM.

Now since the interface is AXI writing a testbench for this in verilog seemed
like a pain and it would be much easier if we could generate the AXI register
writes (and reads) from plain C code. Luckily it is rather easy to interface
a Verilog simulator with your favorite programming language (C of
course) by using VPI (Verilog Procedural Interface).

Using VPI we connect a callback on value change events for the AXI clock signal
and from there we can manipulate the remainder of the bus signals. We have a
simple software driven state machine that, in its idle state, receives commands
from a socket and, whether it was a read or a write, drives the bus signals
accordingly.

Now performing AXI reads and writes in our simulation is a simple matter of
sending the appropriate command packet over the socket and by so we have
achieved a reasonable separation between our simulation environment and the bus
interface to it.

To try it all out follow the instructions below
```
git clone https://github.com/markus-zzz/i2c-controller
cd i2c-controller
./build-all.sh
./run-all.sh
```

In the next post we try to integrate our socket based model with QEMU and later
on in a third post we will attempt to write a Linux device driver for the
EPROM.
