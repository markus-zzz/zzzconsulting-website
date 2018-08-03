---
layout: post
title:  "Building a modem - part 1"
comments: true
categories: hardware rtl verilog modem
---
## Introduction
This is the first post of a new series where we are going to attempt the build
a simple modem. In other words we will try to build a digital communications
link where the sender modulates a communication message, a stream of bytes, to
make it suitable for transmission over a given media and the receiver
demodulates to recover the original message.

Since your author is no expert (to put it mildly) in the fields of
*communications theory* or *signal processing* this post series will not focus
on the theoretical details of that but instead refer to the excellent
presentation of those matters over at [DSP
Illustrations](http://dspillustrations.com).

I envision the complete system to run on a [ZedBoard](http://zedboard.org/) (or
actually two to make it more interesting) and use the onboard audio codec (i.e.
sound waves in the range 20Hz-20kHz) as the transmission media. However the
actual hardware is not really important to follow along as we will rely
extensively on a QEMU / RTL simulation environment.

## Overview
Let's begin by presenting an overview.

### Linux device driver
A Linux device driver presents the device node */dev/zzzmodem*
- *write* - queues a message for transmission. A single call to *write* results
  in a single message queued.
- *read* - returns a received message. A call to *read* returns a single
  received message (i.e. the buffer size given must be equal to the maximal
  transmission unit (MTU)).

Since the *read*/*write* interface is based on complete messages the driver
should allow for multiple processes to *read* and *write* at the same time.

### Modem block memory mapped interface
The modem block exposes two ring buffers in its address map. One for
transmitting (TX) and one for receiving (RX), each of them are accompanied with
read and write pointers.

For TX a device driver *write* will result in one full message being put in
the TX ring buffer. If there is no room then the driver will put the writing
process to sleep. The read pointer updates in one go as the entire message is
sent. An interrupt can be generated whenever the read pointer updates.

For RX a device driver *read* will result in one full message being lifted from
the RX ring buffer. If there are no message to lift then the calling process
will be put to sleep. An interrupt can be generated when the write pointer
updates. The write pointer will only be updated when a full message is put in
the buffer. If the buffer overruns a counter will be incremented and possibly
interrupt generated.

Everything is 32 bit aligned inside the ring buffers. A message begins with a
32 bit header indicating the length of the message. The next message in the
ring buffer will begin (i.e. have its header) at this offset but aligned to a
32 bit boundary.

The following suggested address map of our modem device provides the essentials
and fits well in 4KiB (i.e. 0x1000 bytes).

|Begin|End|Access|Description|
|-----|---|------|-----------|
|0x000|0x3ff|W|TX ring buffer (1 KiB)|
|0x400|0x7ff|R|RX ring buffer (1 KiB)|
|0x800|0x803|R|TX read pointer|
|0x804|0x807|R/W|TX write pointer|
|0x808|0x80b|R/W|RX read pointer|
|0x80c|0x80f|R|RX write pointer|

Access permissions are what one would expect, that is software cannot write to
registers/memory owned by the modem block. All registers should be readable by
software though to ease debugging. Writes to a read-only address are simply
ignored.

### Signal processing
The modulation of choice for our modem will be Quadrature Phase Shift Keying
(QPSK), meaning that each *symbol* will convey two data bits. A *symbol mapper*
will take care of the straight forward task of turning our byte stream message
into a stream of QPSK symbols. For these symbols we then intend to apply the
signal processing chain described
[here](http://dspillustrations.com/pages/posts/misc/baseband-up-and-downconversion-and-iq-modulation.html).

Of course a lot more needs to be said about that and we will constantly return
to this topic as needed. For now though, for the purpose of an overview, this
should be enough.

## Implementation
As there is certainly no shortage of work to be done for this project how do we
decide what to do first?

Well, let us approach this from the top down so I say let's begin with the ring
buffers and their software interface.

To quickly have something that is testable we can set our first goal as
follows: Implement RX and TX ring buffers and tie them together in loop back
mode with some silly action like transforming lowercase letters to uppercase.

Having those parts in place early will hopefully allow us to develop a more
sophisticated testing framework that will come in handy later on when we
approach the more mathematical areas further down the pipe. As well as giving
us additional mileage on our perhaps mundane but oh so error prone byte
shoveling machinery.

That's it for this post! When we return in the next post we will look into the
details of implementing the 'byte shoveling machinery'.

Until then, as usual, if there are questions or feedback - leave a comment
below!

