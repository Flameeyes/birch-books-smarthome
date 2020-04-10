// SPDX-FileCopyrightText: © 2020 The birch-books-smarthome Authors
// SPDX-License-Identifier: MIT
//
// Based on CC-0 licensed demo at
// http://www.colecovision.eu/mcs51/STC89%20DEMO%20BOARD%20LED.shtml

#include <stdbool.h>
#include <stdint.h>

#include <at89x52.h>

/* We can't use C constants for all of this because SDCC is not able to tell
 * that they are static constants, and sets them in RAM instead.
 */

/* The maximum value we care about for the tick clock is 57600 (expanded below),
 * which comes from the formula:
 *
 *    (ticks / second) × (seconds / virtual_hour) × virtual hours
 *
 *         16          ×         225              ×      16        = 57600
 *
 * And we want to force a rollover at that point, to avoid the counter overflow
 * to just mess up the schedule.
 *
 * This has the added benefit of only needing 16-bit for the ticktime.
 */
#define MAXTICK 57600
#define VIRT_HOUR_SECONDS 225

volatile uint16_t ticktime = 0;
volatile bool fastclock = false;

volatile bool rsttestmode = false;
volatile bool testmode = false;

#define CFG_P0OUT 0xFF
// Only 6 bits are configured for output on P2, the other two are inputs.
#define CFG_P2OUT 0x3F

/* Use a very naive approach for the pressed states — sdcc miscompiles complex
 * boolean operations */
#define TEST_PRESSED P2_7
#define FF_PRESSED P2_6

// Schedule for the Birch Books lights.
//
// This assumes the configuration is as follow:
//
// P2 => [NC, NC, 9.1, 9.0, 8 7, 6.2, 6.1]
// P0 => [6.0, 5, 4, 3, 2, 1.2, 1.1, 1.0]

static const unsigned char schedule_p2[16] = {
    0x08, 0x08, 0x08, 0x03, // repeats 6 times
    0x03, 0x03, 0x03, 0x03, 0x03, 0x33, 0x33, 0x33, 0x0C, 0x08, 0x00, 0x00,
};

static const unsigned char schedule_p0[16] = {
    0x30, 0x28, 0x1F, 0x97, // repeats 6 times
    0x97, 0x97, 0x97, 0x97, 0x97, 0x98, 0xF8, 0xF8, 0x07, 0x00, 0x00, 0x00,
};

void clockinc(void) __interrupt(5) {
  /* Debounce the Firing flag. */
  TF2 = 0;

  /* If the main loop set the fastclock flag, increase the tick count by 32.
   *
   * This means that for each 62.5msec we actually record 2 seconds having
   * passed. This "fast forward" should complete the schedule within two minutes
   * rather than an hour.
   */
  uint8_t tickvalue = 1;
  if (fastclock) {
    tickvalue = 32;
  }

  ticktime += tickvalue;

  if (ticktime > MAXTICK)
    ticktime -= MAXTICK;

  /* Use the P1 port for debugging, by setting up the output flags based on some
   * of the firmware's internal flags.
   */
  P1_0 = (bool)fastclock;
  P1_1 = (bool)FF_PRESSED;
  P1_2 = (bool)TEST_PRESSED;
  P1_3 = (bool)rsttestmode;
  P1_4 = (bool)testmode;
}

/* Return the internal timer in seconds.
 *
 * This converts the ticktime value into seconds by dividing by 16 (shifting
 * right 4 bits).
 */
static uint16_t clock(void) {

  EA = 0;

  uint16_t ctmp = ticktime >> 4;

  EA = 1;

  return (ctmp);
}

void main(void) {
  /* Set Timer 2 for auto-reload every 62.5ms.
   *
   * Timer 2 is the only timer with 16-bit auto-reload, making it much easier to
   * set up the timer, as we don't need to manually re-set it.
   *
   * The way you set the T2 timer in 8051-compatible is that you need to set the
   * counter to (65536 - usec), assuming the default behaviour of the timer
   * running at 1MHz (sys_clock/12).
   *
   * The chosen constant (0x0BDB) should execute the timer every 62.5msec, which
   * means it tickets at 1/16th of a second. This allows for a cleaner
   * conversion between ticks and seconds by dividing by 16.
   *
   * You need to set both TH2,TL2 (the current timer) and RCAP2H,RCAP2L (the
   * auto-reset values), to make sure that the timer runs smoothly.
   *
   * Then you need to clear TF2 ("Firing"), both here and in the interrupt
   * routine, and set ET2 ("Enable"), TR2 ("Run"), and EA ("Interrupt Enable").
   */
  TH2 = 0x0B;
  TL2 = 0xDB;
  RCAP2H = 0x0B;
  RCAP2L = 0xDB;

  TF2 = 0;
  ET2 = 1;
  TR2 = 1;
  EA = 1;

  /* Set the basic output ports to zero, to make sure everything starts off.
   */
  P0 = 0x00;
  P1 = 0x00;
  P2 = 0x00;

  /* If FF is pressed at start, consider us in 'self-test mode'.
   *
   * Two self-test mode are implemented:
   *
   *  - If TEST is also pressed, chase a single LED through the output port.
   *  - Otherwise strobe a "knuckle pattern" on the two ports.
   */
  while (FF_PRESSED) {
    rsttestmode = true;
    int clock_secs = clock();
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
      if (clock_secs & 0x01) {
        P0 = 0x55;
        P2 = 0xAA & CFG_P2OUT;
      } else {
        P0 = 0xAA;
        P2 = 0x55 & CFG_P2OUT;
      }
    }
  }

  /* This is the main loop for the firmware.
   *
   * If TEST is pressed, toggle between test mode on or off. Test mode turns on
   * all the of the LEDs at once.
   *
   * Set the `fastclock` flag here in the main loop (so that it doesn't happen
   * during the self-test mode), which increases the ticks in the interrupt.
   */
  for (;;) {
    int clock_secs = clock();
    fastclock = FF_PRESSED;

    if (TEST_PRESSED) {
      testmode = !testmode;
    }

    if (testmode) {
      P0 = CFG_P0OUT;
      P2 |= CFG_P2OUT;
    } else {
      /* The schedule is a 16 "hours" schedule with the two ports setting
       * separate environment.
       */
      int virtual_hour = (clock_secs / VIRT_HOUR_SECONDS) & 0x0F;

      P0 = schedule_p0[virtual_hour];
      P2 = schedule_p2[virtual_hour];
    }

    /* Debounce the TEST keypress. */
    while (TEST_PRESSED)
      ;
  }
}
