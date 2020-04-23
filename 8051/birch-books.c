// SPDX-FileCopyrightText: © 2020 The birch-books-smarthome Authors
// SPDX-License-Identifier: MIT
//
// Based on CC-0 licensed demo at
// http://www.colecovision.eu/mcs51/STC89%20DEMO%20BOARD%20LED.shtml

#include <stdbool.h>
#include <stdint.h>

#include <at89x52.h>

#include "schedule.h"

/* We can't use C constants for all of this because SDCC is not able to tell
 * that they are static constants, and sets them in RAM instead.
 */

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

void clockinc(void) __interrupt(5) {
  /* Debounce the Firing flag. */
  TF2 = 0;

  /* If the main loop set the fastclock flag, increase the tick count by 64.
   *
   * This means that for each 62.5msec we actually record 4 seconds having
   * passed. This "fast forward" should complete the schedule within a minute
   * rather than an hour.
   */
  uint8_t tickvalue = 1;
  if (fastclock) {
    tickvalue = 64;
  }

  /* We let the ticktime overflow transparently, from 65535 back to 0.
   * This simplifies our code quite a bit as we don't need to divide anything.
   */
  ticktime += tickvalue;

  /* Use the P1 port for debugging, by setting up the output flags based on some
   * of the firmware's internal flags.
   */
  P1_0 = (bool)fastclock;
  P1_1 = (bool)FF_PRESSED;
  P1_2 = (bool)TEST_PRESSED;
  P1_3 = (bool)rsttestmode;
  P1_4 = (bool)testmode;
}

/* Return the internal timer in ticks (1/16th of a second).
 *
 * The only difference to accessing ticktime directly is that a copy is made
 * while interrupts are disabled, to avoid it changing during access.
 */
static uint16_t ticks() {
  EA = 0;
  uint16_t ctmp = ticktime;
  EA = 1;

  return ctmp;
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
   *  - If TEST is also pressed, chase a single room through the output port.
   *  - Otherwise strobe a "knuckle pattern" on the two ports.
   */
  while (FF_PRESSED) {
    rsttestmode = true;
    uint16_t clock_secs = ticks() >> 4;
    if (TEST_PRESSED) {
      uint8_t test_index = clock_secs % 10;

      P0 = test_schedule[test_index] & 0xFF;
      P2 = test_schedule[test_index] >> 8;
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
    fastclock = FF_PRESSED;

    if (TEST_PRESSED) {
      testmode = !testmode;

      /* Debounce the TEST keypress.
       *
       * We do this here to make sure we don't see the button-press while we
       * update the outputs. Sounds silly, but at 12MHz it's possible for a
       * human to race the code.
       */
      while (TEST_PRESSED)
        ;
    }

    if (testmode) {
      P0 = CFG_P0OUT;
      P2 = CFG_P2OUT;
    } else {
      /* The schedule is a 16 "hours" schedule with the two ports setting
       * separate environment.
       *
       * We calculate the schedule directly in ticks. There's 256 seconds in a
       * "virtual hour", and since ticks go to 1/16th of a second, this brings
       * us to 4096 ticks per "virtual hour".
       *
       * Unfortunately we need to explicitly express this as a shift, otherwise
       * sdcc generates an integer division function call.
       */
      uint16_t clock_ticks = ticks();
      uint8_t virtual_hour = (clock_ticks >> 12) & 0x0F;

      P0 = schedule[virtual_hour] & 0xFF;
      P2 = schedule[virtual_hour] >> 8;
    }
  }
}
