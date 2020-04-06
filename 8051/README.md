# 8051 Controller and Actuation Board

This implementation of the Birch Books Smart Home code relies on an 8051-based MCU, and is designed with the STC89C52RC in mind (an AT89S52 clone).

## Actuation Board

The Actuation Board is a separate board component that bridges the logic in the controller with the inputs and outputs, and can be used with different controller board designs as well.

On the actuation board, two ULN2003AD Darlington Arrays are connected to 47Ω resistors, and connected to the odd pins of 2x15 connector.

In addition, three pushbuttons provide the input controls: `RST`, `TEST`, and `FF`.

The connection to the MCU is provided via a 1x20 pin header:

| Pin(s) | Description             |
| ------ | ----------------------- |
|   1    | +5V DC                  |
|  2~15  | LED status (HIGH is ON) |
|   16   | FF input (HIGH is ON)   |
|   17   | TEST input (HIGH is ON) |
|   18   | RST input (HIGH is ON)  |
|   19   | Vcc (logic HIGH)        |
|   20   | GND                     |

Note that the connector has both a fixed +5V and a Vcc, to allow controller boards using +3.3V CMOS levels.
