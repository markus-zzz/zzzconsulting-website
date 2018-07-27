---
layout: post
title:  "A basic I2C controller - part 4"
comments: true
categories: hardware rtl verilog i2c qemu linux
---
Now we are going to revisit the device driver from part 3 of this series and
redo it properly.

Modern Linux uses something called a *device tree* to describe the
configuration of a SoC and as the name suggests this data structure describes
what devices are connected to a system. Among the properties described the
following interest us at this point

- identifier for a compatible device driver
- where is this device mapped in memory
- what interrupt line is this device connected to

## Preparations
Since we have chosen to use the ARM VersatileExpress platform for our
experiments we are going to have to modify its *device tree* description which
is contained in the files *arch/arm/boot/dts/vexpress-v2p-ca9.dts* and
*arch/arm/boot/dts/vexpress-v2m.dtsi* (where the former includes the latter).
In fact the former describes the CPU and the latter describes the *motherboard*
i.e. the board that contains all the peripherals.

Since I do not poses a detailed knowledge about this system, for matter such as
what memory regions are available or what interrupt lines are available, the
easiest way forward is going to be to hijack an existing device and reuse its
resources (memory range and interrupt line). Luckily the motherboard has four
UARTs and for our purposes only the first one is needed, so we can safely
borrow the resources from the last one.

In practise that means that we need to make the following modifications
```
diff --git a/arch/arm/boot/dts/vexpress-v2m.dtsi b/arch/arm/boot/dts/vexpress-v2m.dtsi
index b0021a8..f651956 100644
--- a/arch/arm/boot/dts/vexpress-v2m.dtsi
+++ b/arch/arm/boot/dts/vexpress-v2m.dtsi
@@ -179,8 +179,8 @@
                                clock-names = "uartclk", "apb_pclk";
                        };

-                       v2m_serial3: uart@c000 {
-                               compatible = "arm,pl011", "arm,primecell";
+                       zzz_i2c_eprom: zzz_i2c_eprom@c000 {
+                               compatible = "zzz-i2c-eprom";
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
At this point we can (and should) build Linux and boot to note that it finds
one serial port less. Next let's turn our attention to QEMU where we apply the
following change
```
diff --git a/hw/arm/vexpress.c b/hw/arm/vexpress.c
index 9fad791..c1f7550 100644
--- a/hw/arm/vexpress.c
+++ b/hw/arm/vexpress.c
@@ -625,7 +625,11 @@ static void vexpress_common_init(MachineState *machine)
     pl011_create(map[VE_UART0], pic[5], serial_hds[0]);
     pl011_create(map[VE_UART1], pic[6], serial_hds[1]);
     pl011_create(map[VE_UART2], pic[7], serial_hds[2]);
+#if 0
     pl011_create(map[VE_UART3], pic[8], serial_hds[3]);
+#else
     sysbus_create_simple("axi_master_client_device", map[VE_UART3], pic[8]);
+#endif

     sysbus_create_simple("sp804", map[VE_TIMER01], pic[2]);
     sysbus_create_simple("sp804", map[VE_TIMER23], pic[3]);
```
i.e. we map our device to the same address and interrupt as where the fourth
UART used to be.

## Implementation
In RTL the I2C controller has been extended with an IRQ line that asserts
whenever the controller state machine goes from a non-idle state to idle (i.e.
it has just finished work). An AXI mapped register has also been added that,
when written to, de-asserts the IRQ line.

Since our RTL simulation is not that tightly tied into QEMU (and there is some
delay between a de-assert write and the de-asserted IRQ line finds its way back
into QEMU) we are going to use the IRQ line as *edge triggered* to avoid
problems with the ISR falsely triggering.

After rebuilding QEMU and Linux (as described in previous posts) we can now
focus our attention on the actual device driver
[linux/i2c-eprom-driver-irq.c](https://github.com/markus-zzz/i2c-controller/blob/master/linux/i2c-eprom-driver-irq.c).
I would say that it differs from the previous attempt in the following ways

- Registers as a *platform driver* and as such only gets called for *device
  tree* entries that match the given compatibility string
- Gets the interrupt number and mapped memory range from the *device tree* (see
  probe function)
- State machine inside interrupt handler
- Wait queues to block user process while waiting for a file operation to
  finish

During operation the *dev_read*/*dev_write* function does the first register
write to the I2C controller and then puts itself on a wait queue. The state
machine in the ISR will do the remaining register writes to the controller one
at a time. When the last operation has finished the ISR wakes up the sleeping
process.

The wait queue handling deserves some further explanation as to why we cannot
use the more convenient *wait_event_interruptible* and instead have to expanded
the sequence as follows.
```
prepare_to_wait(&wq, &wait, TASK_INTERRUPTIBLE);
/* I2C address device for write mode */
state = S_WRITE_1;
axi_master_write(i2c_ctrl_addr, i2c_ctrl_we_bit | i2c_ctrl_start_bit | I2C_ADDR << 1 | 0 << 0);
schedule();
finish_wait(&wq, &wait);
```
The reason for this is that we need to write the controller command (that will
eventually generate the interrupt so that our ISR wakes us up) before we go to
sleep. However if we for some reason got delayed at the point after the
controller write but before entering our sleep we could have a situation where
the ISR signals us to wake up before we have gone to sleep and then, when we
actually do go to sleep, there will be no ISR to wake us up.

To deal with this situation it is a common pattern to mark ourselves as
sleeping with the *prepare_to_wait* call, then do the controller write and
finally yield with a call to *schedule*. This way there is no harm if the ISR
wakes us up before yielding.

## Wrap up
Now there are certainly more improvements that could be made to the driver and
especially in the area of concurrent access to the device but I think that we
are going to be happy with the current state for this post. To be honest the
driver for this device feels a bit contrived and I would rather revisit this
topic with a more realistic device (e.g. some sort of communications device
with ring buffers for rx and tx).

Did you like this post? Questions or feedback - leave a comment below!
