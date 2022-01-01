---
layout: post
title:  "Attending Matt Venn's ZeroToASIC course"
math: true
comments: true
categories: hardware
---

A few months ago I decided to sign up for Matt Venn's ZeroToASIC course and now
I plan to write a short summary describing the experience. For those not
already familiar Matt started this initiative about a year ago and has since
then been producing informative videos at the [Zero To ASIC Course YouTube
channel](https://www.youtube.com/zerotoasic). After having watched a bunch of
those videos I finally gave in bought a ticket for the course. While I
originally found its $650 to be a bit on the pricey side one has to think about
all the work that goes into running this as a full time job and besides I
really think it is an effort worth supporting.

SKY130 and OpenLane

## Digital

I found the digital part to be very straight forward to follow along but you
really have to respect the amount of preparation work that goes into
configuring and setting the tools in a usable state. Sure all the tools and
information is openly available but I would have easily spent vast amounts of
time doing so and probably given up long before it reached the state they are
offered in for the course.

For the project part I originally planned to use the MyC64 SID implementation
for the groups submission targeting MPW3 but in the end this did not end up
happening. For the groups submission each submitter has a die area of 300x300um
and the SID implementation barely fit in that space. To be honest, and this was
also what was recommended, to get the most out of the project part of the
course you should really focus on a dead simple design so you can clearly see
what is going on during each step of the RTL to GDS2 flow. Needless to say the
SID implementation did not fall into that category but instead turns into a
spaghetti bowl of gates and flip-flops after synthesis.

Besides even if I went ahead with this there would still be the question of
D/A-conversion. Of course a simple R-2R converter could be placed on the board
but that did not seem like much fun. Another option could be to interface some
off the shelf I2S D/A-converter chip but that seemed like rather high risk of
not working in the end and being hard to debug.

## Analogue

As a collaboration with [Thomas
Parry](https://www.zerotoasiccourse.com/post/livestream-with-thomas-parry/) an
analogue add-on course four about $200 was also offered.
