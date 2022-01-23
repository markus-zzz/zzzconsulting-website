---
layout: post
title:  "Analogue IC design"
math: true
comments: true
categories: hardware
---

Here is a short story about me having an attempt at analogue integrated circuit
(IC) design in SkyWater's 130nm process. For those who do not know
[SKY130](https://skywater-pdk.readthedocs.io/en/main/) is a collaboration
between Google and SkyWater to provide a open Physical Design Kit (PDK) that
can be manufactured at SkyWater's fabs. Further Google is sponsoring several
yearly Multi Project Wafer (MPW) submissions via [efabless](https://efabless.com/).

During early fall of 2021 I heard of [Matt Venn's ZeroToASIC
course](https://www.zerotoasiccourse.com) which is a practical course using
SKY130 and the [OpenLane](https://github.com/The-OpenROAD-Project/OpenLane)
flow to cover all steps from RTL to MPW submission. It did not take long before
I decided to sign up. Shortly after an add-on course on Analoge design in
collaboration with [Thomas
Parry](https://www.zerotoasiccourse.com/post/livestream-with-thomas-parry/) was
also offered. I decided to sign up for that too.

While I am not an electrical engineer by training, and hence lacking a fair bit
of the theory required for successful circuit design, I found the process using
open source tools very interesting. In the flow used by Thomas
[xschem](https://github.com/StefanSchippers/xschem) is used for schematic
capture, [ngspice](http://ngspice.sourceforge.net/) for simulation and
[klayout](https://www.klayout.de/) for the final mask layout (GDS2 editing).

Towards the end of the course I had a go at a 5 transistor op-amp.

[Insert 5topamp schematic]

say something about current mirrors and diff pair

and then doing the layout

![Final layout](/download/analogue-ic/layout-7.png)

say something about the layout especially the size of the compensation capacitor.

[Insert picture of layout as part of dub-dub's submission]

link to MPW3 submission at efabless and say something about when we expect to get silicon.
