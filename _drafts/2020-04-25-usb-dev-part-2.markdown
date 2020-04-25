---
layout: post
title:  "USB device part-2"
math: true
comments: true
categories: hardware
---

### Clock recovery
The signaling in USB consists of the differential pair D+ and D-. For a *full
speed (FS)* device the bit rate is 12Mbit/s so if we were able to sample at
exactly the right spot a 12Mhz clock should suffice, in reality though this is
the tricky bit. To aid in clock recovery USB employs both NRZI encoding and
bit-stuffing to ensure that the differential pair will contain a level
transition at least every seven bit times. 

With this in mind it would seem a reasonable approach to run the design at
48Mhz and oversample the differential pair with a factor of four. More
precisely by obtaining four equally spaced samples for each bit time we should
be able to adjust the real sampling position (in terms of 1/4 bit times) to be
as far away for any level transition as possible (i.e. in the middle of the eye
diagram).

So running at a 48Mhz clock we have a 2-bit counter (`reg [1:0] cntr`)
incrementing each cycle (except for when adjusting). When the counter equals
zero we perform the real sample. For every 48Mhz cycle we also sample and shift
the value into a four bit shift register (`reg [3:0] past`). Since we want any
possible level transition to occur in the middle of this shift register we
either advance or delay the counter with one increment depending on if a
transition occurred early or late in the shift register.
```
  // A bit transition should ideally occur between past[2] and past[1]. If it
  // occurs elsewhere we are either sampling too early or too late.
  assign advance = past[3] ^ past[2];
  assign delay   = past[1] ^ past[0];
```
If `advance` is active the counter increments by two and if `delay` is active
there is no increment (for the given 48Mhz cycle).

After seeing some transitions this should be able to adjust the sampling point
to where the signal lines are stable.
