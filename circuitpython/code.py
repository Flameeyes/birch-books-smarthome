# SPDX-FileCopyrightText: © 2020 The birch-books-smarthome Authors
# SPDX-License-Identifier: MIT

import time

import board
import busio

from adafruit_mcp230xx.mcp23016 import MCP23016
from adafruit_mcp230xx.mcp23017 import MCP23017

import schedule

i2c = busio.I2C(board.SCL, board.SDA)

i2c.try_lock()
i2c_devices = i2c.scan()
i2c.unlock()

if 32 in i2c_devices:
    mcp = MCP23016(i2c)
    print("Found MCP23016 @ 32")
elif 33 in i2c_devices:
    mcp = MCP23017(i2c, address=33)
    print("Found MCP13017 @ 33")
else:
    raise Exception("No GPIO Expander found.")

mcp.iodir = 0xC000

test_pin = mcp.get_pin(15)
ff_pin = mcp.get_pin(14)


def write_gp(value):
    mcp.gpio = value & 0x3FFF


while ff_pin.value:
    print("Entering on-reset test loop.")
    clock_secs = int(time.monotonic())

    if test_pin.value:
        write_gp(schedule.TEST_SCHEDULE[clock_secs % 10])
    else:
        if clock_secs % 2:
            write_gp(0x5555)
        else:
            write_gp(0xAAAA)
    time.sleep(0)

test_mode = False
virtual_hour = None
counter = 0
while True:
    counter += 1
    time.sleep(0)
    if test_pin.value:
        test_mode = not test_mode
        print("Test mode: %s" % test_mode)
        if test_mode:
            virtual_hour = None
            write_gp(0xFFFF)

        while test_pin.value:
            continue

    if test_mode:
        time.sleep(0)
        continue

    if ff_pin.value:
        seconds_per_virtualhour = 4
    else:
        seconds_per_virtualhour = 240

    monotonic = int(time.monotonic())
    monotonic_hour = monotonic // seconds_per_virtualhour

    new_virtual_hour = monotonic_hour % 16
    if virtual_hour == new_virtual_hour:
        continue
    virtual_hour = new_virtual_hour

    print(
        "It's %d o'clock (%d %d) and all is well."
        % (virtual_hour, monotonic_hour, monotonic)
    )

    write_gp(schedule.SCHEDULE[virtual_hour])
