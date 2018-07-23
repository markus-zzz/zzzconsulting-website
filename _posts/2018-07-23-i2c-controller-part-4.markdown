---
layout: post
title:  "A basic I2C controller - part 4"
comments: true
categories: hardware rtl verilog i2c qemu linux
---
Now we are going to revisit the device driver from part 3 of this series and
redo it properly.

Modern linux uses something called a *device tree* to describe the
configuration of a SoC and as the name suggests this data structure describes
what devices are connected to a system. Among the properties described the
following interest us at this point

- identifier for a compatible device driver
- where is this device mapped in memory
- what interrupt line is this device connected to

Since we have chosen to use the ARM VersatileExpress platform for our
experiments we are going to have to modify its *device tree* description with
is contained in the files *arch/arm/boot/dts/vexpress-v2p-ca9.dts* and
*arch/arm/boot/dts/vexpress-v2m.dtsi* where the former includes the latter. In
fact the former describes the CPU and the latter describes the *motherboard*
i.e. the board that contains all the peripherals.

Since I do not poses a detailed knowledge about this system, for matter such as
what memory regions are available or what interrupt lines are available, the
easiest way forward is going to be to hijack an existing device and reuse its
resources (memory range and interrupt line). Luckily the motherboard has four
UARTS and for our purposes only the first one is needed so we can safely borrow
resources from the last one.

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
At this point we can build and boot Linux and note that it finds one serial
port less. Next let's turn our attention to QEMU where we apply the following
change
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
