# SPDX-FileCopyrightText: 2020 The birch-books-smarthome Authors
#
# SPDX-License-Identifier: MIT

MCU=atmega48
CFLAGS=-Wall -mcall-prologues -mmcu=$(MCU) -Os
LDFLAGS=-Wl,-gc-sections -Wl,-relax
CC=avr-gcc

all: birch-books.ihx

clean:
	rm -f *.o *.ihx

%.ihx: %.o
	avr-objcopy -R .eeprom -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
