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

The attentive reader might realise that the approach of having the RTL
simulator block on a socket inside the value change callback for the AXI clock
will effectively block the simulation unless it is constantly being fed with
command packets over the socket. As a lucky coincidence this stream of socket
commands is exactly what happens in the test suite used in this post as it
constantly busy waits after each write to the control register.

Later on though when we use QEMU and a proper interrupt driven device driver
this will present a problem so we might as well try to solve it now. On the
opposite side of blocking we could do the receive in non-blocking mode and
effectively have the RTL simulation running at full speed all the time. While
this would be functionally correct it is somewhat undesirable since it would be
a huge wast of simulation cycles (and possibly producing some huge dump.vcd
files).

A better approach is to do a bit of both with the concept of a clock request
signal from the block (we can use the busy bit of the status register for this
purpose). While the block is busy it needs a clock to finish (and become ready)
and in this state we use a non-blocking receive. On the other hand when the
block is ready it is safe to fall back on blocking receives as there is no work
to do until we get a command from the AXI.

The code for this post series is found [here](https://github.com/markus-zzz/i2c-controller).

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
