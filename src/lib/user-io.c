#include "user-io.h"

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <util/delay.h>

/* LED (digital pin 13) on port B */
#define PORTB_LED (1 << 5)

/* Button on digital pin 12 port B */
#define PORTB_BUTTON_0 (1 << 4)

/* Buzzer (digital pin 2) on port D */
#define PORTD_BUZZER (1 << 2)

/* Button on digital pin 3 port D */
#define PORTB_BUTTON_1 (1 << 3)

/* Button minimum hold time (ms) -- avoid counting bounces as presses */
#define BUTTON_HOLD_TIME_MS 20

volatile uint8_t changedbits;
volatile uint8_t portbhistory = 0xFF;

/* Structure to track button presses */
struct button_info {
	uint8_t hold_time; /* Time the button was held down */
	uint8_t count; /* Number of times the button was pressed */
};


/* Static functions */
static void track_button(struct button_info* info);
static uint8_t get_tracked_presses(const struct button_info* info);


/* Initializes the LED/button interface. */
void init_led_button(void)
{
	/* Configure LED as output, buzzer as output, button as input */
	DDRB |= PORTB_LED;
	DDRD |= PORTD_BUZZER;
}

/* Initialize interrupts */
void
init_interrupts(void)
{
	/* register button pins as inputs */
	DDRB &= ~PORTB_BUTTON_0 & ~PORTB_BUTTON_1;

	/* enable pullup on button pins */
	PORTB |= PORTB_BUTTON_0 | PORTB_BUTTON_1;


	/* turn on interrups for port B pins and mask the pins connected to buttons */
	cli();
	PCICR |= 0b00000001;
	PCMSK0 |= PORTB_BUTTON_0 | PORTB_BUTTON_1;
	sei();
}

/* Interrupt routine which saves the bits that were changed on Pin B */
ISR(PCINT0_vect)
{
	changedbits = PINB ^ portbhistory;
	portbhistory = PINB;
}

/* Wait for the confirm button to be pressed */
void
wait_for_confirm(void)
{
	while (1) {
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_mode();

		if (changedbits & PORTB_BUTTON_1 && portbhistory & PORTB_BUTTON_1) {
			beep(1);
			break;
		}
	}
}

/*
 * Select a number by pressing the accumulator button n times and the pressing
 * the confirm button
 */
uint8_t
select_number(void)
{
	uint8_t val = 0;

	while (1) {
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_mode();

		if (changedbits & PORTB_BUTTON_0 && portbhistory & PORTB_BUTTON_0) {
			beep(0);
			val++;
		}

		if (changedbits & PORTB_BUTTON_1 && portbhistory & PORTB_BUTTON_1) {
			beep(1);
			break;
		}
	}

	_delay_ms(200);

	for (uint8_t i = 0; i < val; ++i) {
		beep(2);
		_delay_ms(100);
	}


	return val;
}

/* Wait the specified amount of time for the button to be pressed. */
bool wait_for_button_timeout(uint16_t led_on_time_ms, uint16_t led_off_time_ms,
	uint16_t timeout_ms)
{
	const uint16_t led_cycle_time_ms = led_on_time_ms + led_off_time_ms;
	uint16_t led_cycle_pos = 1;
	struct button_info info = {0, 0};

	while (timeout_ms > 0) {
		if (led_cycle_pos == 1) {
			PORTB |= PORTB_LED;
		} else if (led_cycle_pos == led_on_time_ms) {
			PORTB &= ~PORTB_LED;
		} else if (led_cycle_pos == led_cycle_time_ms) {
			led_cycle_pos = 0;
		}

		track_button(&info);

		if (info.count) {
			break;
		}

		timeout_ms -= 1;
		led_cycle_pos += 1;
	}

	/* Will wait for the button to be released */
	uint8_t presses = get_tracked_presses(&info);
	PORTB &= ~PORTB_LED;

	return presses > 0;
}


/* Blink the LED and wait for the user to press the button. */
uint8_t count_button_presses(uint16_t led_on_time_ms,
	uint16_t led_off_time_ms)
{
	const uint16_t led_cycle_time_ms = led_on_time_ms + led_off_time_ms;
	uint16_t led_cycle_pos = 1;
	struct button_info info = {0, 0};
	uint16_t timeout_ms = 0;

	while ((info.count == 0) || (timeout_ms > 0)) {
		if (led_cycle_pos == 1) {
			PORTB |= PORTB_LED;
		} else if (led_cycle_pos == led_on_time_ms) {
			PORTB &= ~PORTB_LED;
		} else if (led_cycle_pos == led_cycle_time_ms) {
			led_cycle_pos = 0;
		}

		track_button(&info);

		if (info.hold_time) {
			timeout_ms = 500;
		}

		timeout_ms -= 1;
		led_cycle_pos += 1;
	}

	PORTB &= ~PORTB_LED;

	/* Will wait for the button to be released */
	return get_tracked_presses(&info);
}

/* Wait a fixed amount of time, blinking the LED */
uint8_t delay(uint16_t led_on_time_ms, uint16_t led_off_time_ms,
	uint16_t delay_ms)
{
	uint16_t led_cycle_time_ms = led_on_time_ms + led_off_time_ms;
	uint16_t led_cycle_pos = 1;
	struct button_info info = {0, 0};
	uint16_t remaining = delay_ms;

	while (remaining > 0) {
		if (led_on_time_ms != 0) {
			if (led_cycle_pos == 1) {
				PORTB |= PORTB_LED;
			} else if (led_cycle_pos == led_on_time_ms) {
				PORTB &= ~PORTB_LED;
			} else if (led_cycle_pos == led_cycle_time_ms) {
				led_cycle_pos = 0;
			}
		}

		track_button(&info);

		remaining -= 1;
		led_cycle_pos += 1;
	}

	PORTB &= ~PORTB_LED;

	if (delay_ms <= BUTTON_HOLD_TIME_MS) {
		/* The wait delay is lower than the minimum hold time, so
		   get_tracked_presses will not return a correct number of press times.
		   Instead, we just return 1 if the button was held at all. */

		return (info.hold_time > 0) ? 1 : 0;
	}

	/* Will wait for the button to be released */
	return get_tracked_presses(&info);
}


/*
 * Emit a brief beep the buzzer.
 *
 * Multiple tones can be customized depending on the on/off delay
 */
void
beep(uint16_t tone)
{
	for (uint8_t i = 0; i < 50 / (tone + 1); ++i) {
		PORTD |= PORTD_BUZZER;
		switch (tone)
		{
		case 0:
			_delay_ms(1);
			break;
		case 1:
			_delay_ms(2);
			break;
		case 2:
			_delay_ms(2);
			break;
		}

		PORTD &= ~PORTD_BUZZER;

		switch (tone)
		{
		case 0:
			_delay_ms(1);
			break;
		case 1:
			_delay_ms(1);
			break;
		case 2:
			_delay_ms(2);
			break;
		}

	}
}


/*
 * Track the button presses during roughly 1 ms.
 * The info struct must be initialized to all zeros before calling this
 * function.
 */
void track_button(struct button_info* info)
{
	bool button_held = ((PINB & PORTB_BUTTON_0) == 0);

	if (button_held) {
		/* The button is held; increment the hold time */
		info->hold_time += 1;

	} else {
		/* Check if the button was just released after being held for
		   a sufficient time */
		if (info->hold_time > BUTTON_HOLD_TIME_MS) {
			info->count += 1;
		}

		info->hold_time = 0;
	}

	_delay_ms(1);
}


/*
 * Count the button presses after a tracking operation.
 */
uint8_t get_tracked_presses(const struct button_info* info)
{
	uint8_t count = info->count;

	/* Wait for the button to be released */
	while ((PINB & PORTB_BUTTON_0) == 0) {
		/* Nothing */
	}

	/* Count the last button press (if the button was still held the last time
	   track_button was called */
	if (info->hold_time > BUTTON_HOLD_TIME_MS) {
		count += 1;
	}

	return count;
}
