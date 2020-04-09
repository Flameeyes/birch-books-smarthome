// SPDX-FileCopyrightText: © 2020 The birch-books-smarthome Authors
// SPDX-License-Identifier: MIT
//
// Based on CC-0 licensed demo at
// http://www.colecovision.eu/mcs51/STC89%20DEMO%20BOARD%20LED.shtml

#include <stdbool.h>

#include <at89x52.h>

volatile unsigned long int clocktime;
volatile bool clockupdate;

volatile bool testmode = false;

#define CFG_P0OUT 0xFF
// Only 6 bits are configured for output on P2, the other two are inputs.
#define CFG_P2OUT 0x3F

#define CFG_P2TEST 0x80
#define CFG_P2FF 0x40
#define CFG_P2IN CFG_P2TEST | CFG_P2FF

#define TEST_PRESSED (P2 & CFG_P2TEST)
#define FF_PRESSED (P2 & CFG_P2FF)

void clockinc(void) __interrupt(1)
{
	TH0 = (65536 - 922) / 256;
	TL0 = (65536 - 922) % 256;
	if (FF_PRESSED) {
	  clocktime += 1000;
	} else {
	  clocktime++;
	}
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

	while (FF_PRESSED) {
	  int clock_secs = clock() / 100000;
	  if (TEST_PRESSED) {
	    int bit = clock_secs % 14;
	    if (bit < 8) {
	      P0 = 1 << bit;
	      P2 = 0;
	    } else {
	      bit -= 8;
	      P2 = 1 << bit;
	      P0 = 0;
	    }
	  } else {
	    if (clock_secs % 2) {
	      P0 = 0x55;
	      P2 = 0xAA & CFG_P2OUT;
	    } else {
	      P0 = 0xAA;
	      P2 = 0x55 & CFG_P2OUT;
	    }
	  }
	}

	for(;;) {
	  int clock_secs = clock() / 1000;

	  if (TEST_PRESSED) {
	    testmode = !testmode;
	  }

	  if (testmode) {
	    P0 = CFG_P0OUT;
	    P2 |= CFG_P2OUT;
	  } else {
	    P0 = (clock_secs) & CFG_P0OUT;
	    P2 = (clock_secs >> 8) & CFG_P2OUT;
	  }

	  while (TEST_PRESSED);
	}
}
