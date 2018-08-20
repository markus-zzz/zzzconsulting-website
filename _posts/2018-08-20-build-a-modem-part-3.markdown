---
layout: post
title:  "Building a modem - part 3"
math: true
mermaid: true
comments: true
categories: hardware rtl verilog modem
---
## Introduction
The current state of our modem project is that we have the infrastructure (ring
buffers, bus interface and device driver) to pass a sequence of bytes (octets)
into the TX path of our modem RTL design and similarly retrieve a sequence of
bytes from the RX path.

This means that we can now turn out attention to the [the signal processing
chain](http://dspillustrations.com/pages/posts/misc/baseband-up-and-downconversion-and-iq-modulation.html)
and the details on what should happen inside the TX and RX paths. After
carefully studying the previous link we begin to outline our design

## Where are we?

### Signal processing path
If we temporarily limit ourselves to only output a baseband signal we can
rather quickly identify the following blocks and what kind of data
(octet, symbol or sample) that flow between them
<div class="mermaid">
graph TD
subgraph Transmit Path
  T0[Tx Ring]
  T1[Symbol Mapper]
  T2[Up Converter]
  T3[RRC Filter]
  T4[DAC]

  T0-->|Octets|T1
  T1-->|Symbols|T2
  T2-->|Samples|T3
  T3-->|Samples|T4
end
subgraph Receive Path
  R0[ADC]
  R1[RRC Filter]
  R2[Timing Recovery]
  R3[Symbol Sampler]
  R4[Symbol DeMapper]
  R5[Rx Ring]


  R0-->|Samples|R1
  R1-->|Samples|R2
  R1-->|Samples|R3
  R3-->|Symbols|R4
  R4-->|Octet|R5

  R2-->|Adjustments|R3
end
</div>

- **ADC** - Analog to Digital Converter.
- **DAC** - Digital to Analog Converter.
- **RRC Filter** - Root Raised Cosine filter for symbol shaping. A matched
  filter is used on the RX path.
- **Timing Recovery** - Gardner algorithm to adjust the symbol sampling position.
- **Symbol Sampler** - Every N:th sample marks the center of a symbol.
- **Symbol DeMapper** - Maps received symbols to QPSK symbols and when four
  have been gathered put them in an octet.
- **Symbol Mapper** - Maps an octet into four QPSK symbols.
- **Up Converter** - Inserts a symbol every N:th sample.

### Frame format
At this point it is also useful to quickly outline what the frame format is
expected to look like

|Field|Bits|Description|
|-----|----|-----------|
|Preamble|32|Alternating pattern that allows the receiver to do timing recovery and adjust the symbol sampling point|
|SyncWord|16|Synchronization pattern to indicate that the symbols should from now on be collected in octets and passed on to the MAC layer for further processing|
|Length|8|Length of payload|
|Payload||Data payload|
|CRC|16|16-bit Cyclic Redundancy Checksum over the payload|

## Let's do something!
### Root Raised Cosine (RRC) Filter

The major block missing the complete the TX path is the RRC filter so we will
try to implement that first.

on these octets. As can be seen the signal processing involves arithmetic on
real or complex numbers (a pair of reals) and hence we need a way to represent
these non-integer quantities in our design. Now this is fairly straight forward
if your platform supports floating point numbers but since we are implementing
the hardware ourselves we will have to do without. The standard solution for
this is to use a fixed point representation and we will look into that shortly.

Picking up from where we left of last time means that first we will need to to
split the octets into symbol sized chunks. Since we will be using QPSK that
means that we will need a *splitter* to split each byte into four two bit
pairs. Each bit pair is then trivially mapped into a QPSK symbol represented as
a complex fixed point pair.

Next looking at the signal processing chain it should become evident that we
will need to apply several FIR filters

- A Root Raised Cosine (RRC) filter at the transmitter for pulse shaping (complex)
- A matched RRC filter at the receiver to complete the pulse shaping (complex)
- A low pass filter at the receiver to go from passband to baseband (real?)

## Fixed point representation
First I have a look at [this]({{site.url}}/download/building-a-modem/fp.pdf)
excellent write up on the topic of fixed point representation its arithmetic.
Following the notation of that paper we will be using 16 bit integers in the
A(2,13) format.

This means that the range of representable numbers is
$$ 0 \leq x \le 2^N$$

and that the accuracy (the maximum error between our representation and the
real number we are actually trying to represent) is
$$ 1/2^N $$

## Convolution
Our FIR filters will be realized by convolving the input signal with a filter
kernel. The *convolution* operator is given by

$$ y(n)=(x*h)[n]=\sum_{k=-\infty}^{\infty}x(k)h(n-k)$$

The fact that we are targeting an audio link means that the sample rate will
be substantially lower than the internal clock frequencies of the design and that
allows us to go with a rather naive and economical convolutor design. We will be using

- Two multipliers (real and imaginary)
- One single ported synchronous ROM block for the filter coefficients $$h(n)$$
- One single ported synchronous RAM block for the input signal $$x(n)$$

A state machine controls the usage of the resources. A *valid* *ready* flow
control mechanism is used on both the input and output side of the block.

1. When a new $$x(n)$$ sample is accepted it is written into the SPRAM at the
   position of the write pointer.
1. Multiplication starts sequentially multiplying, with filter coefficients,
   from write pointer to write pointer minus number of filter coefficients.
1. Multiplication finished and the write pointer is incremented.




## Gardner algorithm for timing recovery
