// SPDX-FileCopyrightText: © 2020 The birch-books-smarthome Authors
// SPDX-License-Identifier: MIT
//
// Based on CC-0 licensed demo at
// http://www.colecovision.eu/mcs51/STC89%20DEMO%20BOARD%20LED.shtml

#include <stdbool.h>

#include <at89x52.h>

volatile unsigned long int clocktime;
volatile _Bool clockupdate;

void clockinc(void) __interrupt(1)
{
	TH0 = (65536 - 922) / 256;
	TL0 = (65536 - 922) % 256;
	clocktime++;
	clockupdate = true;
}

unsigned long int clock(void)
{
	unsigned long int ctmp;

	do
	{
		clockupdate = false;
		ctmp = clocktime;
	} while (clockupdate);

	return(ctmp);
}

void main(void)
{
	// Configure timer for 11.0592 Mhz default SYSCLK
	// 1000 ticks per second
	TH0 = (65536 - 922) / 256;
	TL0 = (65536 - 922) % 256;
	TMOD = 0x01;
	IE |= 0x82;
	TCON |= 0x10; // Start timer

	P0 = 0x00;
	P2 = 0x00;

	for(;;) {
	  int clock_secs = clock() / 1000;

	  /*
	  if (P3 & 0x10) {
	    P0 = ~0xFF;
	    P2 = P2 & 0xEC;
	  } else */ {
	    P0 = (clock_secs) & 0xFF;
	    P2 = (clock_secs >> 8) & 0xFF;
	  }
	}
}
