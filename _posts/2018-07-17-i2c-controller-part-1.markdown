---
layout: post
title:  "A basic I2C controller - part 1"
comments: true
categories: hardware rtl verilog i2c
---
## Introduction
I2C is this two wire bus protocol that is very common in the embedded sphere
and the reader is most likely already familiar with it so the introduction here
will be brief.  As the name suggests it is a protocol for communicating between
ICs and the specification for the protocol can be found
[here](http://cache.nxp.com/documents/user_manual/UM10204.pdf).  The two wires
consist of a clock (SCL) and a data (SDA), the master is responsible for
initiating all transactions. Since it is a bit more complicated than just a
shift-register there are special rules for when *SDA* transitions may occur
with respect to *SCL* (i.e. not just data sampled at rising edge of clk
end-of-story). For example:

- A SDA 1 -> 0 transition when SCL is high signals a START condition
- A SDA 0 -> 1 transition when SCL is high signals a STOP condition
- SDA transitions when SCL is low are used for normal data signaling

Of course there are more details to it but you can read all about those in the
spec above.

In this post we are going to outline the RTL (Verilog) implementation and
simulation of a basic I2C controller that connects to a host as an AXI slave. In
future posts we will connect the simulation with QEMU and finish it all up by
writing a Linux device driver.

## Implementation
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

If one thinks a bit more about the signaling one realises that dividing the SCL
into four phases might be a good idea so we use a clock enable scheme as
follows:

```
// I2C clocking scheme
//
//        -+                             +-----------------------------+
// SCL     |                             |                             |
//         +-----------------------------+                             +-
//
//        -+            +-+            +-+            +-+            +-+
// 4x_en   |            | |            | |            | |            | |
//         +------------+ +------------+ +------------+ +------------+ +-
//
// phase      2'b00          2'b01          2'b10          2'b11
```
In other words we use the same clock domain as the AXI bus and 'divide' the clock
locally by producing a clock enable signal with four pulses per *SCL* period. For
each pulse we increment the two bit phase counter that is used by the remainder
of the design.

The implementation of the controller is in
[i2c_controller.v](https://github.com/markus-zzz/i2c-controller/blob/master/i2c_controller.v).

Next thing we need to wrap an AXI slave interface around the controller and
that can be seen in
[i2c_axi_slave.v](https://github.com/markus-zzz/i2c-controller/blob/master/i2c_axi_slave.v).

## Simulation
For testing I found
[this](https://github.com/olofk/i2c/blob/master/bench/verilog/i2c_slave_model.v)
slave model of a I2C EPROM.

Now if you think about the amount of AXI signaling that would be required to
write and read something from the EPROM model you quickly realise that it would
be a pain to write all that in plain Verilog. In fact it would be much easier
if we could write all this in a programming language like C and luckily thanks
to VPI (Verilog Procedural Interface) this is a straight forward task.

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
constantly busy-waits on the status register after each write to the control
register.

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

The code for this post series can be found in
[https://github.com/markus-zzz/i2c-controller](https://github.com/markus-zzz/i2c-controller).

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
