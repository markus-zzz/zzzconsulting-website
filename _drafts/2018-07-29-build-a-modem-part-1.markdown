---
layout: post
title:  "Building a modem - part 1"
comments: true
categories: hardware rtl verilog modem
---
## Introduction
This is the first post of a new series where we are going to attempt the build
a simple modem. In other words we will build a digital communications link
where the sender modulates the communication message, stream of bytes (or octets
as they are often called in this context), to make it suitable for transmission
over a given media (e.g. airwaves or audio) and the receiver demodulates to
recover the original message.

Since your author is no expert (to put it mildly) in the fields of
*communications theory* or *signal processing* this post series will serve as
an educational journey for me as well.

I envision the complete system to run on a ZedBoard (or actually two to make it
more interesting) and use the onboard audio codec (i.e. sound waves 20Hz-20kHz) as
the transmission media. However the actual hardware is not really important to
follow along as we will rely extensively on a QEMU / RTL simulation environment
(just like we did in the I2C controller series).

## Overview
Let's begin by presenting an overview.

### Linux device driver
A Linux device driver presents the device node */dev/modem*
- *write* - queues a message for transmission. A single call to *write* results
  in a signel message queued.
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

The entire address map of our modem device is 4KiB (i.e. 0x1000 bytes) let us
internally divide it as follows

|Begin|End|Access|Description|
|-----|---|------|-----------|
|0x000|0x3ff|W|TX ring buffer (1 KiB)|
|0x400|0x7ff|R|RX ring buffer (1 KiB)|
|0x800|0x803|R|TX read pointer|
|0x804|0x807|R/W|TX write pointer|
|0x808|0x80b|R/W|RX read pointer|
|0x80c|0x80f|R|RX write pointer|

The access permissions are what one would expect, that is software cannot write
to registers/memory owned by the modem block. All registers should be readable by
software though to ease debugging. Writes to a read only address are simply
ignored.

### Modulation
The modulation of choice will be Quad Phase Shift Keying (QPSK) which means
that a *symbol* will correspond to two data bits.

### Pulse shaping
A Root Raised Cosine (RRC) filter will be used for pulse shaping. I and Q are
pulse shaped separately and then output as baseband. Note that at this point we
can actually loop back this into the RX path and continue working on those
parts before we concern ourselves with the issues of transforming baseband to
passband.

## Implementation
Let us start off by trying implement the ring buffer interface and see how far
we get in this post before it is time to wrap up.

To have something that is testable we can set the following as our first goal:
'Implement RX and TX ring buffers and tie them together in loop back mode with
some silly action like transforming lowercase letters to uppercase and test
with device driver'. Actually that sounds like quite a lot of work so it will
likely be a few posts down the road before we reach that goal.
