<!--
SPDX-FileCopyrightText: 2020 The birch-books-smarthome Authors

SPDX-License-Identifier: MIT
-->

# Board Description

For the Birch Books Smart Home project I decided to design the system with two type of
boards: an actuation board, and a controller board. The reason for the split design is
that the controller board can be replaced with alternative implementations without
requiring a new set of wiring for the LEDs.

## Actuation Board

The Actuation Board is a separate board component that bridges the logic in the
controller with the inputs and outputs, and can be used with different controller board
designs as well.

The actuation board is as simple as it can be:

  * 3× pushbuttons (`RST`, `TEST`, `FF`);
  * 3× 1kΩ resistors for pull-down;
  * 2× ULN2003AD Darlington Array;
  * 14× 47Ω resistors, or 2× bussed 7-port resistor networks, for the LEDs.

Two connectors are also present: a 1x20, breadboard compatible header to connect to the
controller board, and a 2×15 connector to connect to the LEDs.

### MCU Connector

| Pin(s) | Description             |
| ------ | ----------------------- |
|   1    | +5V DC                  |
|  2~15  | LED status (HIGH is ON) |
|   16   | FF input (HIGH is ON)   |
|   17   | TEST input (HIGH is ON) |
|   18   | RST input (HIGH is ON)  |
|   19   | VIO (logic HIGH)        |
|   20   | GND                     |

Note that the connector has both a fixed +5V and a VIO, to allow controller boards using
+3.3V CMOS levels for input/output.

As the board was originally designed for 8051 MCUs, the RST line is *active HIGH*, which
is unusual. To connect it to an active-low RST pin, consider using a NPN transistor.

## Controller Boards

Multiple controller boards are present in this repository, with different level of test.

### 8051/8052 (Partly Tested)

This is the original controller board. It was designed with an STC89C52 MCU in mind (a
modern 8052 implementation), and as such it has the UART marked as `ISP_STC` on board
itself.

It includes an optionally-populated CP2104 USB-to-UART chip (and related passives) to
make the programming easier. But it has not been tested since I failed to program a new
chip with the correct firmware.

The firmware for this board is in `8051/`.

### Trinket M0 with MCP23016/17 (In Use)

This version of the controller is designed to work with an [Adafruit Trinket
M0](https://www.adafruit.com/product/3500) board on top.

There are two separate boards in the directory, which are based around the same design
and firmware, but with different GPIO Expanders on them. It's recommended to use the
**MCP23017** version, which has two fewer components and is more reliable.

Both boards have marking and can be technically adapted to use any MCU that supports I²C
and has drivers for the MCP23016/17 series adapters. The needed lines are +5V, VIO (e.g.
3.3V), GND, SDA, SCL, ¬RST.

The firmware for this board is in `circuitpython/`.

### ATmega48

This version of the controller is designed to work with
[ATmega48](https://www.microchip.com/wwwproducts/en/ATmega48) and
[ATmega328](https://www.microchip.com/wwwproducts/en/ATmega328) (the latter untested).

This board is a compromise between the 8051 and Trinket M0 options: it include a simple
8-bit MCU, requiring no passive components, but also an inverter for the RST line.

The ISP header should be compatible with many ATmega programmers, as it exposes the SPI
bus together with the reset and power lines, and is configured as follows:

| Pin | Description |
| --- | ----------- |
|  1  |     SIO     |
|  2  |     +5V     |
|  3  |     SCK     |
|  4  |     SDO     |
|  5  |     RST     |
|  6  |     GND     |

Note that there is no CS line.

The firmware for this board is in `atmega48/` and should be **source** compatible with
ATmega328, but might require Makefile changes.
