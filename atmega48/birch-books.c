// SPDX-FileCopyrightText: © 2020 The birch-books-smarthome Authors
// SPDX-License-Identifier: MIT

#define F_CPU 1000000

#include <stdbool.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "schedule.h"

volatile uint16_t ticktime = 0;
volatile bool fastclock = false;

volatile bool testmode = false;

#define TEST_PRESSED (PINB & (1 << PINB1))
#define FF_PRESSED (PINB & (1 << PINB0))

ISR(TIMER1_COMPA_vect) {

  /* If the main loop set the fastclock flag, increase the tick count by 64.
   *
   * This means that for each 62.5msec we actually record 4 seconds having
   * passed. This "fast forward" should complete the schedule within a minute
   * rather than an hour.
   */
  uint8_t tickvalue = fastclock ? 64 : 1;

  /* We let the ticktime overflow transparently, from 65535 back to 0.
   * This simplifies our code quite a bit as we don't need to divide anything.
   */
  ticktime += tickvalue;
}

/* Return the internal timer in ticks (1/16th of a second).
 *
 * The only difference to accessing ticktime directly is that a copy is made
 * while interrupts are disabled, to avoid it changing during access.
 */
static uint16_t ticks() {
  cli();
  uint16_t ctmp = ticktime;
  sei();

  return ctmp;
}

int main() {
  /* Set ports C (0~5) and D (0~7) for output. */
  DDRC |= ((1 << DDC0) | (1 << DDC1) | (1 << DDC2) | (1 << DDC3) | (1 << DDC4) |
           (1 << DDC5));
  DDRD |= ((1 << DDD0) | (1 << DDD1) | (1 << DDD2) | (1 << DDD3) | (1 << DDD4) |
           (1 << DDD5) | (1 << DDD6) | (1 << DDD7));

  /* Run a LED self-test at start, for just a second. Do this before setting up
   * timers, so that the timer starts counting from 0. */
  PORTC = 0xFF;
  PORTD = 0xFF;
  _delay_ms(1000);

  /* Set ports to zero. */
  PORTC = 0;
  PORTD = 0;

  /* Set up Timer 1 for 62.5ms. */
  OCR1A = 62499;            // 1/16th of a second at 1MHz clock.
  TCCR1B |= ((1 << CS10)    // Fcpu prescale = 1
             | (1 << WGM12) // CTC mode
  );
  TIMSK1 |= (1 << OCIE1A); // Enable CTC Interrupt
  sei();                   // Enable global interrupts

  /* If TEST is pressed at start, consider us in 'self-test mode': chase a
   * single room through the output port.
   */
  while (TEST_PRESSED) {
    uint16_t clock_secs = ticks() >> 4;
    uint8_t test_index = clock_secs % 10;

    PORTC = test_schedule[test_index] & 0xFF;
    PORTD = test_schedule[test_index] >> 8;
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
      PORTC = 0xFF;
      PORTD = 0xFF;
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

      PORTC = schedule[virtual_hour] & 0xFF;
      PORTD = schedule[virtual_hour] >> 8;
    }
  }
}
