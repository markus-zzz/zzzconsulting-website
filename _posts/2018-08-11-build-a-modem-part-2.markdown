---
layout: post
title:  "Building a modem - part 2"
comments: true
categories: hardware rtl verilog modem
---
## Introduction
In this post we return to our modem building project to checkup on the
progress.

The code corresponding to the progress of this post is available as branch
[part-2](https://github.com/markus-zzz/modem/tree/part-2) of the modem GitHub
repository.

### Hardware blocks
The current implementation exposes two AXI mapped ring buffers, and some
control register, using the address map from the [previous
post]({{site.baseurl}}{% post_url 2018-08-03-build-a-modem-part-1 %}).

Each ring buffer is currently implemented using a dual ported synchronous SRAM
block with one port being tied to the AXI and the other to the internal data
path of the modem.

Now dual ported memories are large and expensive and for the modest data rates
we plan to put on these memories we can certainly get a way both AXI and
internal data path sharing the single port on a single ported memory. Doing so
is of course not free and it will be at the expense of some increased
implementation complexity but that we can handle later. So in other words, for
now dual ported is good since they allow for slightly simpler implementation
but we should eventually switch to using single ported.

Either way, dual ported or not, Synchronous SRAM blocks have one slightly
annoying property with respect to reads and that is that the read address is
clocked. That is the behavioural model looks like this
```
    // read operation
    always @(posedge rclk) begin
        if (rce) begin
            ra <= raddr;
        end
    end

    assign do = mem[ra];
```
In other words you cannot simply present the read address (*raddr*) and expect
to latch the data (*do*) at the next rising clock edge but instead you have to
wait an additional cycle. Of course it is all pretty obvious when you think
about it (they are called synchronous for a reason) but I for one tend to
forget this ever so often.

One place where this shows up is in the AXI read channel response logic in
[rtl/verilog/modem_axi_slave.v](https://github.com/markus-zzz/modem/blob/part-2/rtl/verilog/modem_axi_slave.v)
since we have to delay the read response for ring buffer reads by one cycle
compared to if it was a register read.

Another thing that slightly complicates matters is that we cannot simply drain
the TX ring buffer at constant full speed but instead need to repeatedly do
stop and go to avoid overflowing the internal data path as it operates at a
much lower speed (one byte is several symbols, one symbol is several samples,
etc, etc).

To deal with this we need a *handshake* mechanism, and we have chosen to do one
using two signals, *valid* and *ready*, as follows.
- Producer asserts *valid* when valid data is output
- Consumer asserts *ready* when it can accept data
- The transaction occurs on a rising clock edge when both *ready* and *valid*
  are sampled high

Right now the TX and RX ring buffers are tied together in loop-back mode with
the silly action of transforming lower-case letters to upper-case and vice
versa.

An interrupt pulse is generated when the hardware updates the read pointer of
the TX ring and the write pointer of the RX ring.

See [rtl/verilog/modem_axi_top.v](https://github.com/markus-zzz/modem/blob/part-2/rtl/verilog/modem_axi_top.v)

#### TX-Ctrl
This is the controller for the transmit ring buffer. It is basically a state
machine that sits around waiting for the read pointer to be become unequal the
write pointer (indicating that the buffer is no longer empty) and then starts
reading words.

The fact that the internal data path needs to be fed with bytes, the pipelined
read behaviour of the memory and the presence of flow control slightly
complicates things. This in combination with the long term ambition to switch
to a single ported memory results in the current simple design, with perhaps a
seemingly abundant number of states.

See [rtl/verilog/tx_ctrl.v](https://github.com/markus-zzz/modem/blob/part-2/rtl/verilog/tx_ctrl.v)

#### RX-Ctrl
This is the controller for the receive ring buffer. It is quite similar to its
transmit counterpart although it operates in the opposite direction and as a
result does not have to deal with the issues of memory reads.

One thing we do have to deal with here however is the detection of overflows,
but that is rather straight forward and we simply skip writing the message
header and advancing the write pointer when this condition is detected.
Eventually we should set a status bit indicating the overflow in the header of
the next message written to the ring and have the device driver informing
user-space somehow (perhaps by one *read* returning *EOVERFLOW*).

See [rtl/verilog/rx_ctrl.v](https://github.com/markus-zzz/modem/blob/part-2/rtl/verilog/rx_ctrl.v)

### Linux device driver
The current device driver resembles the one from the I2C series quite a lot. A
*read* operation will block if the RX ring is empty and a *write* operation
will block if the TX ring has insufficient space left for the message. In
either case the sleeping process will be awoken by previously mentioned
interrupt pulse (indicating that the HW has updated its pointers).

This implementation seems to work for now but later on should probably add
mutexes to protect the ring buffers from concurrent access (i.e. many processes
doing *read* at the same time and similarly for *write*). Not sure if that
use-case make a ton of sense though but we should properly handle that
situation anyway.

Also we would likely benefit from a more fine grained interrupt control, being
able to see if the source was RX or TX and being able to mask.

For details see
[linux/zzz-modem-driver.c](https://github.com/markus-zzz/modem/blob/part-2/linux/zzz-modem-driver.c)

### Try it out!
```
git clone https://github.com/markus-zzz/modem.git
cd modem
git checkout part-2
```
As a sanity check run the simple standalone test transmitting a sequence of
test messages from size 4 to size 16
```
./build-all.sh && ./run-all.sh
```
After that we can move on to do testing with QEMU and the Linux device driver.
The steps for setting this up are almost identical to what was described in the
I2C controller series (especially [part-2]({{site.baseurl}}{% post_url
2018-07-18-i2c-controller-part-2 %}) and [part-4]({{site.baseurl}}{% post_url
2018-07-27-i2c-controller-part-4 %})) so repetion here will be brief.

Modify the Linux device tree for our modem device
```
diff --git a/arch/arm/boot/dts/vexpress-v2m.dtsi b/arch/arm/boot/dts/vexpress-v2m.dtsi
index b0021a8..2279196 100644
--- a/arch/arm/boot/dts/vexpress-v2m.dtsi
+++ b/arch/arm/boot/dts/vexpress-v2m.dtsi
@@ -179,8 +179,8 @@
                                clock-names = "uartclk", "apb_pclk";
                        };

-                       v2m_serial3: uart@c000 {
-                               compatible = "arm,pl011", "arm,primecell";
+                       zzz_modem: zzz_modem@c000 {
+                               compatible = "zzz-modem";
                                reg = <0x0c000 0x1000>;
                                interrupts = <8>;
                                clocks = <&v2m_oscclk2>, <&smbclk>;
diff --git a/arch/arm/boot/dts/vexpress-v2p-ca9.dts b/arch/arm/boot/dts/vexpress-v2p-ca9.dts
index 5814460..98206c0 100644
--- a/arch/arm/boot/dts/vexpress-v2p-ca9.dts
+++ b/arch/arm/boot/dts/vexpress-v2p-ca9.dts
@@ -25,7 +25,6 @@
                serial0 = &v2m_serial0;
                serial1 = &v2m_serial1;
                serial2 = &v2m_serial2;
-               serial3 = &v2m_serial3;
                i2c0 = &v2m_i2c_dvi;
                i2c1 = &v2m_i2c_pcie;
        };
```
For QEMU we need to be using
[qemu/axi_master_client_device.c](https://github.com/markus-zzz/modem/blob/part-2/qemu/axi_master_client_device.c)
and not the corresponding file from the previous repository.

Once everything is built and moved to the proper places the RTL simulation is
started with
```
vvp -M. -mvpi_axi_master modem.vvp
```
and QEMU as before. Watch Linux boot, then login, install the module and do the
following

```
cat /dev/zzz-modem > rx.txt &
echo "Hello World!" > /dev/zzz-modem
cat rx.txt
```
And to transmit some more messages of varying length
```
for i in $(seq 100); do echo "Hello $(seq -s " " $i)" > /dev/zzz-modem; done
```
### What is next?

Right now we have implemented the loop-back at the highest level possible, i.e.
right after the ring buffers. This is nice, in my opinion, since it allows for
easy testing with the */dev/zzz-modem* device node right away and gives us more
time to flush out any annoying bugs in the HW/SW interface.

Since our implementation strategy is to go top down we will continue to do so
and keep on moving the loop-back loop further down the stack as more layers are
added.

Currently I imagine that the following milestones will be useful

- Next have loop-back of IQ symbols
- Then loop-back of sampled pulse shaped IQ symbols
- Then introduce a shift so that we need to do synchronization in RX
- Do the same thing as previous step but use passband instead of baseband
- ...
- Finally, a physical audio loop-back wire on the ZedBoard

That is it for this post. Questions or feedback - leave a comment below!
