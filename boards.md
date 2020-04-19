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
  * 2× ULN2003AD Darlington Array;
  * 14× 47Ω resistors, or 2× bussed 7-port resistor networks.

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
|   19   | Vcc (logic HIGH)        |
|   20   | GND                     |

Note that the connector has both a fixed +5V and a Vcc, to allow controller boards using
+3.3V CMOS levels.
