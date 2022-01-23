---
layout: post
title:  "Making some PCBs - part 2 (HyperRAM)"
math: true
mermaid: true
comments: true
categories: hardware
---

# Stage 1 - HyperRAM
2-layer board.

First out is a HyperRAM expansion board for the ULX3S. It makes sense to see
early if I will be able to pull off BGA assembly with the equipment I have.
Also I happen to have 10 of these
[S27KL0641DABHI020](https://www.mouser.se/ProductDetail/cypress-semiconductor/s27kl0641dabhi020/)
laying around.

5x5 BGA means that the two outermost rings can be routed in the same layer
(only one ball remaining in the middle but that can also escape in the same
layer as one corner is missing a ball).

I finally had a go at this and put together quick and dirty PCB in KiCAD and
had <https://aisler.net/> manufacture it for me. By quick and dirty I mean that
the layout is ugly and the traces are not length matched so I would not expect
to get any performance out of it. The board should be functionally correct
though. I did not bother to put the KiCad files on GitHub.

Two different soldering methods were evaluated and I tried capturing the chip
profile with a cheap cell-phone camera through a binocular microscope.

On the left only tacky flux was applied and then using hot air from above
(station set to 300C at 50l/min) the chip was soldered in place. After a while
the chip noticeably sunk into place and I tried to nudge it several times with
a pair of tweezers after which it immediately bounced back. This was a quick
method but the end result is that the chip sits flush with the board and visual
inspection is not really possible.

On the right no flux was used but instead solder paste was applied through a
stencil. The board was placed on a hot-plate that slowly rose to a temperature
set to 220C after which hot air was applied from above (with station set to
300C at 40l/min). It was not as noticeable when the chip sunk into place and I
did not dare to nudge it (as it was still on the hot-plate). The end result
looks much nicer though with the solder balls clearly visible and one can
clearly see that there is no bridging.

![HyperRAM BGA](/download/pcb/hyperram-bga-pcb.jpg)

Eventually I will try to verify the boards with one of the controllers from the
list below and report the results.

* <https://github.com/blackmesalabs/hyperram> - slow but portable controller
* <https://github.com/gregdavill/litex-hyperram> - full performance controller for ECP5 in Migen
* <https://1bitsquared.com/products/pmod-hyperram>
* <https://www.youtube.com/watch?v=dThZDl-QG5s>

I did a second try run using the stencil after having acquired
<https://www.oshstencils.com/> excellent PCB fixture jig. To me having the
board properly held in place by jig helped a lot when applying the paste. Of
course one can get a similar effect using a few scrap PCBs but I don't really
have many of those lying around that are the right size and besides these
brackets just cost a few dollars.

As far as I can tell the stencil alignment and paste coverage turned out well
and the end result looks good. So with this I feel reasonably confident that I
will also be able to handle the larger FPGA BGA with its 0.8mm pitch (the
HyperRAM BGA has a 1.0mm pitch). Again I still have not gotten around to
actually verify that these modules work so maybe I should not say too much.

![HyperRAM BGA second attempt](/download/pcb/hyperram-bga-pcb-2.jpg)


## Verification

Now a few monhts later I felt sufficiently energized to have a go at testing
the HyperRAM boards.  I set up [a test
bench](https://github.com/markus-zzz/hyperram-test) using [the portable
HyperRAM controller from
blackmesalabs](https://github.com/blackmesalabs/hyperram) and the
[PicoRV32](https://github.com/YosysHQ/picorv32) RISC-V CPU core.

The DRAM is mapped into the CPU's address space presenting its 8MB as 2M 32-bit
dwords.

The test consists of having the CPU write the Fibonacci sequence, as 32-bit
integers, to memory and then verifying that it reads back correctly. Test
result is presented by lighting the appropriate LEDs.

All three boards have been fully assembled and the test passes on all three.

It should be noted that this is a basic test running the memory at a low
frequency (6.25MHz) so it does not stress test the board in any way. It does
however provide some confidence in that the part survived the assembly process,
the solder joints are reasonably sound and that there is no bridging.

![Testing the HyperRAM board](/download/pcb/hyperram-test-pcb.jpg)

## Future

It could be interesting to improve the controller, setting a shorter latency,
or simply switch to using a different one.
