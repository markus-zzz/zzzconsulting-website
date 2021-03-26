---
layout: post
title:  "A most basic GPU - part 1"
math: true
mermaid: true
comments: true
categories: hardware rtl verilog gpu
---
## Introduction

Putting the MyC64 project on hold for a while.

Recently I have seen updates on the PS1 FPGA core by
[@Laxer3A](https://twitter.com/laxer3a), which I of course cant help finding
interesting. I have never owned a PS1 and honestly never cared much about it
but it turns out that graphics wise it had have some interesting design
choices/limitations. More preciecely the graphics pipeline is based around
fixed point representation so there are no floating point numbers at all.
Further more there is no z-buffer. Modern Vintage Gamer has put together a
short video describing these limitations that is well worth a watch as an
introduction - [Why PlayStation 1 Graphics Warped and Wobbled so much |
MVG](https://www.youtube.com/watch?v=x8TO-nrUtSI).

So apparently it was possible to reach commercial success with a device like
this some 25 years ago. Nowadays probably not so much but it is still
interesting to see that one can get away with these limitations and still get
reasonable results.

For me this suggests a very basic GPU may actually be within reach as a fun
FPGA project.

The rasterization algorithm is very well described in [Rasterization: a
Practical
Implementation](https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation)
over at [Scratch Pixel](https://www.scratchapixel.com/). What follows below
will be some thoughts that I had while reading that and preparing for a design
that could be put inside a FPGA.

## Project scope

* Fixed pipeline (duh, not likely to implement programmable shaders any time soon)
* Fixed point integers only (no floating point)
* Flat shading only (triangles filled with solid color)
* No z-buffer (geometry has to be sorted before sent down the pipeline)

## Rasterization stage

This stage is about finding out, for a given triangle, what screen pixels are
covered by that triangle. Since well behaved triangles contain significantly
more pixels than vertices (three) this step is likely to be the main bottle
neck of the system.

Determining if a pixel is inside a triangle is done by checking if it falls on
the *right* side of all three edges.

Intuitively it seems that the cross product could be used to determine if a
point is on the left-side or right-side of an edge. Now screen space is 2d and
the cross product is a 3d only concept but one can of course limit the vectors
to be contained in the plane $$ z=0 $$. In that case the well known cross
product

$$ a \times b = (a_2b_3-a_3b_2, a_3b_1-a_1b_3, a_1b_2-a_2b_1) $$

simply becomes

$$ a_1b_2-a_2b_1 $$

which in rasterization is known as an *edge function*. It is worth noting that
the *edge function* is in fact the magnitude of the cross product of the two vectors
in the $$ z=0 $$ plane (so it has something to do with area).

So the *edge function* for edge $$ u \to v $$ and arbitrary point $$ p $$ is

$$ E_{u \to v}(p) =  (v_x - u_x)(p_y - u_y) - (v_y - u_y)(p_x - u_x) $$

now condsider what happens if an offset $$ o $$ is applied

$$\begin{eqnarray}
E_{u \to v}(p+o) &=& (v_x - u_x)(p_y + o_y - u_y) - (v_y - u_y)(p_x + o_x - u_x) \\
                 &=& E_{u \to v}(p) + (v_x - u_x)o_y - (v_y - u_y)o_x
\end{eqnarray}$$

in other words once we have computed $$ E_{u \to v}(p) $$ we can explore
neighbouring pixels by simply adding and subtracting $$ (v_x - u_x) $$ and $$
(v_y - u_y) $$ both of which we have already computed for $$ E_{u \to v}(p) $$.

This is a significant improvement as it allows us to replace two
multiplications and five additions with a single addition.

Naively every pixel of the screen need to be checked against each triangle for
coverage but obviously one often do much better by simply checking the bounding
box of the triangle. Either way the amount of pixels is significant so this
last result is really important as it will allow us to check multiple pixels
each cycle at a reasonable cost in hardware.

The idea is to consider quads of 2x2 pixels. Begin by computing the $$ E_{u \to
v}(p) $$ for the pixel in the upper left corner of the bounding box. After that
add/subtract as described above to get the remaining three pixels of the quad.
This should preferably be done in sequence not to wast too much resources. Once
the first quad is done the neighbouring quad in $$ x $$ direction is obtained
by simple addition of the precomputed $$ 2(v_y - u_y) $$ (keeping in mind that
we get the shift by one for free in hardware by simply hard-wiring in a zero at
the lsb).

Still though every triangle has three edges so evaluating a quad every cycle
means that 12 adders need to be instantiated. If that results in too high
resource utilization we may need to cut down a bit.
