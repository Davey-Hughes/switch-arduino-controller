/*
 * Pokémon Brilliant Diamond/Shining Pearl automation
 */

#include <util/delay.h>

#include "automation-utils.h"
#include "user-io.h"

/* Static functions */
static void release_box_sequence(void);
static void release_box(void);
static void release_pokemon(void);
static void hatch_eggs_sequence(void);
static void farm_eggs_sequence(void);
static void swap_eggs(uint8_t);
static void leave_PC_at_PC(void);
static void go_to_PC_at_PC(void);
static void wait_for_another_egg(void);
static void get_egg(void);
static void move_to_daycare_man(void);
static void fly_to_solaceon(void);
static void temporary_control(void);
static void move_in_circles(uint16_t cycles);
static void hatch_eggs(uint16_t cycles);

static uint8_t CUR_BOX, MAX_BOX, CUR_COL, MAX_COL;


int
main(void)
{
	init_automation();
	init_led_button();
	init_interrupts();

	/* initial beep to confirm that the buzzer works */
	beep(2);

	/* on start of the arduino, the accumulator button can be pressed to
	 * intitiate giving control to the real controller. this is useful when
	 * the arduino was restarted abruptly while a script was running */
	if (select_number() > (uint8_t) 0) {
		switch_controller(VIRT_TO_REAL);
		wait_for_confirm();
	}

	switch_controller(REAL_TO_VIRT);

	set_leds(BOTH_LEDS);
	pause_automation();

	while (1) {
		/* Set the LEDs, and make sure automation is paused while in the menu */
		set_leds(BOTH_LEDS);
		pause_automation();

		uint8_t program = select_number();
		switch (program) {
		case 0:
			farm_eggs_sequence();
			break;
		case 1:
			hatch_eggs_sequence();
			break;
		case 2:
			release_box_sequence();
			break;
		default:
			delay(100, 200, 1500);
			break;
		}
	}
}


/*
 * Release multiple boxes
 *
 * Must start with the cursor over the first pokemon in the first box
 */
static
void
release_box_sequence(void)
{
	uint8_t num_boxes = select_number();

	for (uint8_t i = 0; i < num_boxes; ++i) {
		release_box();
	}

	temporary_control();
}


/*
 * Release an entire box of pokemon
 *
 * Box must be full of pokemon. There is no detection for empty spaces!
 */
static
void
release_box(void)
{
	/* traverse box left and right alternating */
	for (uint8_t i = 0; i < 5; ++i) {
		uint8_t direction = i % 2 == 0 ? DP_RIGHT : DP_LEFT;

		for (uint8_t k = 0; k < 5; ++k) {
			release_pokemon();

			SEND_BUTTON_SEQUENCE(
				{ BT_NONE,	direction,	SEQ_HOLD,	1 },
				{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6 },
			);
		}

		release_pokemon();

		SEND_BUTTON_SEQUENCE(
			{ BT_NONE,	DP_BOTTOM,	SEQ_HOLD,	1 },
			{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6 }
		);

	}

	/* at the end, move the cursor to the first pokemon in the next box */
	SEND_BUTTON_SEQUENCE(
		{ BT_R,		DP_NEUTRAL,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	12 },
		{ BT_NONE,	DP_BOTTOM,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6  },
		{ BT_NONE,	DP_BOTTOM,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6  },
		{ BT_NONE,	DP_RIGHT,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6  },
		{ BT_NONE,	DP_RIGHT,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6  }

	);
}


/*
 * Release a single pokemon under the cursor in the PC
 */
static
void
release_pokemon(void)
{
	SEND_BUTTON_SEQUENCE(
		{ BT_A,		DP_NEUTRAL,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6  },
		{ BT_NONE,	DP_TOP,		SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6  },
		{ BT_NONE,	DP_TOP,		SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6  },
		{ BT_A,		DP_NEUTRAL,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6  },
		{ BT_NONE,	DP_TOP,		SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6  },
		{ BT_A,		DP_NEUTRAL,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	20 },
		{ BT_A,		DP_NEUTRAL,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6  }
	);
}


/*
 * Hatch eggs from the PC
 */
void
hatch_eggs_sequence(void)
{
	/* convert offcial egg cycles to approximate cycles in the script
 	 * needed to hatch eggs */
	uint16_t egg_cycles[] = {
		180,	/*  5 */
		0,	/* 10 */
		0,	/* 15 */
		590,	/* 20 */
		0,	/* 25 */
		800,	/* 30 */
		0,	/* 35 */
		0	/* 40 */
	};

	/* select number of boxes to hatch */
	MAX_BOX = select_number();

	/* select number of egg cycles to hatch for (multiples of 5) */
	uint8_t num_egg_cycles = -1;
	while (num_egg_cycles > sizeof(egg_cycles)) {
		num_egg_cycles = select_number();
	}

	CUR_BOX = 0;
	CUR_COL = 0;
	MAX_COL = 5;

	fly_to_solaceon();

	while (CUR_BOX < MAX_BOX) {
		go_to_PC_at_PC();
		swap_eggs(CUR_COL);
		leave_PC_at_PC();

		hatch_eggs(egg_cycles[num_egg_cycles]);
	}

	temporary_control();
}


/*
 * Farm eggs from the daycare man
 */
static
void
farm_eggs_sequence(void)
{
	/* multiple of 30 to approximate number of boxes to fill with eggs */
	uint16_t cycles = select_number() * 30;

	for (uint16_t i = 0; i < cycles; ++i) {
		if (i % 10 == 0) {
			fly_to_solaceon();
			move_to_daycare_man();
		}

		get_egg();
		wait_for_another_egg();
	}

	temporary_control();
}


/*
 * Move to an open position in Solaceon and run around in circles to hatch eggs
 */
static
void
hatch_eggs(uint16_t cycles)
{
	/* move to the left and down from the Pokemon Center and get on the bike */
	SEND_BUTTON_SEQUENCE(
		{ BT_B,		DP_LEFT,	SEQ_HOLD,	24 },
		{ BT_B,		DP_BOTTOM,	SEQ_HOLD,	40 },
		{ BT_P,		DP_NEUTRAL,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	10 },
	);

	move_in_circles(cycles);

	/* press A enough to get through 5 eggs hatching */
	for (uint16_t i = 0; i < 135; ++i) {
		SEND_BUTTON_SEQUENCE(
			{ BT_A,		DP_NEUTRAL,	SEQ_HOLD,	1  },
			{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	20 }
		);
	}

	fly_to_solaceon();
}


/*
 * Interact with the PC to swap 5 lower pokemon in party to the PC at the
 * specified column
 */
static
void
swap_eggs(uint8_t column)
{
	/* press A to enter the menu to move pokemon */
	for (uint8_t i = 0; i < 5; ++i) {
		SEND_BUTTON_SEQUENCE(
			{ BT_A,		DP_NEUTRAL,	SEQ_HOLD,	1  },
			{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	25 }
		);
	}

	/* switch to "quick mode" */
	SEND_BUTTON_SEQUENCE(
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	10 },
		{ BT_Y,		DP_NEUTRAL,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6  }
	);


	/* move the cursor to the left */
	SEND_BUTTON_SEQUENCE(
		{ BT_NONE,	DP_LEFT,	SEQ_HOLD,	1 },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6 }
	);

	/* move the cursor to a pokemon in the box, select it, then move it to
 	 * the party */
	for (uint8_t k = 0; k < 5; ++k) {
		/* this if/else handles chooing the most efficient direction
 		 * for exchanging pokemon */
		if (CUR_COL < 3) {
			for (uint8_t i = 0; i < column + 1; ++i) {
				SEND_BUTTON_SEQUENCE(
					{ BT_NONE,	DP_RIGHT,	SEQ_HOLD,	1 },
					{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6 }
				);
			}
		} else {
			for (uint8_t i = 0; i < MAX_COL - CUR_COL + 1; ++i) {
				SEND_BUTTON_SEQUENCE(
					{ BT_NONE,	DP_LEFT,	SEQ_HOLD,	1 },
					{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6 }
				);
			}
		}

		/* after moving the cursor, select the pokemon in the box with A */
		SEND_BUTTON_SEQUENCE(
			{ BT_A,		DP_NEUTRAL,	SEQ_HOLD,	1 },
			{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6 }
		);

		if (CUR_COL < 3) {
			for (uint8_t i = 0; i < column + 1; ++i) {
				SEND_BUTTON_SEQUENCE(
					{ BT_NONE,	DP_LEFT,	SEQ_HOLD,	1 },
					{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6 }
				);
			}
		} else {
			for (uint8_t i = 0; i < MAX_COL - CUR_COL + 1; ++i) {
				SEND_BUTTON_SEQUENCE(
					{ BT_NONE,	DP_RIGHT,	SEQ_HOLD,	1 },
					{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6 }
				);
			}
		}

		/* move the cursor to the correct position in the party and
 		 * exchange the party pokemon for an egg */
		SEND_BUTTON_SEQUENCE(
			{ BT_NONE,	DP_BOTTOM,	SEQ_HOLD,	1 },
			{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6 },
			{ BT_A,		DP_NEUTRAL,	SEQ_HOLD,	1 },
			{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	6 }
		);
	}

	CUR_COL++;

	/* switch to next box for the next time the PC is interacted with if
 	 * we're at the end of the current box */
	if (CUR_COL > MAX_COL) {
		CUR_COL = 0;
		SEND_BUTTON_SEQUENCE(
			{ BT_R,		DP_NEUTRAL,	SEQ_HOLD,	1  },
			{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	25 }
		);

		CUR_BOX++;
	}

	/* leave the PC menu */
	for (uint8_t i = 0; i < 5; ++i) {
		SEND_BUTTON_SEQUENCE(
			{ BT_B,		DP_NEUTRAL,	SEQ_HOLD,	1  },
			{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	25 }
		);
	}

}


/*
 * From the PC inside the Pokemon Center, run to outside the Pokemon Center
 */
static
void
leave_PC_at_PC(void)
{
	SEND_BUTTON_SEQUENCE(
		{ BT_B,		DP_BOTTOM,	SEQ_HOLD,	10 },
		{ BT_B,		DP_LEFT,	SEQ_HOLD,	14 },
		{ BT_B,		DP_BOTTOM,	SEQ_HOLD,	80 },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	70 }
	);
}


/*
 * From the front of the Pokemon Center, run to the PC inside
 */
static
void
go_to_PC_at_PC(void)
{
	SEND_BUTTON_SEQUENCE(
		{ BT_B,		DP_TOP,		SEQ_HOLD,	130 },
		{ BT_B,		DP_RIGHT,	SEQ_HOLD,	14  },
		{ BT_B,		DP_TOP,		SEQ_HOLD,	10  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	4   }
	);
}


/*
 * Run around next to the daycare man for a set period of time for an egg to
 * appear
 */
static
void
wait_for_another_egg(void)
{
	/* move to the left of the daycare man */
	SEND_BUTTON_SEQUENCE(
		{ BT_B,		DP_LEFT,	SEQ_HOLD,	6  },
		{ BT_P,		DP_NEUTRAL,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	4  },
	);

	/* wait for a set amount of time */
	move_in_circles(50);

	/* move back to the daycare man */
	SEND_BUTTON_SEQUENCE(
		{ BT_P,		DP_NEUTRAL,	SEQ_HOLD,	1  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	4  },
		{ BT_B,		DP_LEFT,	SEQ_HOLD,	15 },
		{ BT_B,		DP_BOTTOM,	SEQ_HOLD,	10 },
		{ BT_B,		DP_TOP,		SEQ_HOLD,	6  },
		{ BT_B,		DP_RIGHT,	SEQ_HOLD,	10 },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	4 }
	);
}


/*
 * Talk to the daycare center man for an egg
 *
 * Strategic pressing of the B button prevents getting stuck in the dialogue
 * when an egg is not available
 */
static
void
get_egg(void)
{
	for (uint8_t i = 0; i < 8; ++i) {
		SEND_BUTTON_SEQUENCE(
			{ BT_A,		DP_NEUTRAL,	SEQ_HOLD,	1  },
			{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	20 }
		);
	}

	for (uint8_t i = 0; i < 6; ++i) {
		SEND_BUTTON_SEQUENCE(
			{ BT_B,		DP_NEUTRAL,	SEQ_HOLD,	1  },
			{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	20 }
		);
	}
}


/*
 * Move to the left side of the daycare man from the front of the Solaceon PC
 */
static
void
move_to_daycare_man(void)
{
	SEND_BUTTON_SEQUENCE(
		{ BT_B,		DP_LEFT,	SEQ_HOLD,	33  },
		{ BT_B,		DP_TOP,		SEQ_HOLD,	26  },
		{ BT_B,		DP_LEFT,	SEQ_HOLD,	32  },
		{ BT_B,		DP_BOTTOM,	SEQ_HOLD,	10  },
		{ BT_B,		DP_TOP,		SEQ_HOLD,	6   },
		{ BT_B,		DP_RIGHT,	SEQ_HOLD,	10  },
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	4   }
	);
}


/*
 * Fly to solaceon town
 * Character must be outside of a building
 */
static
void
fly_to_solaceon(void)
{
	SEND_BUTTON_SEQUENCE(
		{ BT_X,		DP_NEUTRAL,	SEQ_HOLD,	1  },	/* open menu */
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	25 },	/* wait for menu */
		{ BT_P,		DP_NEUTRAL,	SEQ_HOLD,	1  }	/* open map */
	);

	for (int i = 0; i < 100; ++i) {
		send_update(BT_NONE, DP_NEUTRAL, S_BOTRIGHT, S_NEUTRAL); /* move cursor to bottom right map */
	}

	for (int i = 0; i < 11; ++i) {
		send_update(BT_NONE, DP_NEUTRAL, S_TOP, S_NEUTRAL); /* move cursor to Solaceon City */
	}

	for (int i = 0; i < 15; ++i) {
		send_update(BT_NONE, DP_NEUTRAL, S_LEFT, S_NEUTRAL); /* move cursor to Solaceon City */
	}

	send_update(BT_NONE, DP_NEUTRAL, S_NEUTRAL, S_NEUTRAL); /* stop cursor */

	SEND_BUTTON_SEQUENCE(
		{ BT_A,		DP_NEUTRAL,	SEQ_HOLD,	1   },	/* select Solaceon City */
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	25  },	/* wait for menu */
		{ BT_A,		DP_NEUTRAL,	SEQ_HOLD,	1   },	/* select yes */
		{ BT_NONE,	DP_NEUTRAL,	SEQ_HOLD,	180 }	/* wait to fly */
	);
}


/*
 * Move the left stick in a circle
 */
static
void
move_in_circles(uint16_t cycles)
{
	set_leds(RX_LED);

	for (uint16_t i = 0; i < cycles; ++i) {
		switch (i % 4) {
		case 0:
			set_leds(NO_LEDS);
			break;
		case 1:
			set_leds(TX_LED);
			break;
		case 2:
			set_leds(BOTH_LEDS);
			break;
		case 3:
			set_leds(RX_LED);
			break;
		}

		send_update(BT_NONE, DP_NEUTRAL, S_RIGHT, S_NEUTRAL);
		send_update(BT_NONE, DP_NEUTRAL, S_TOPRIGHT, S_NEUTRAL);
		send_update(BT_NONE, DP_NEUTRAL, S_TOP, S_NEUTRAL);
		send_update(BT_NONE, DP_NEUTRAL, S_TOPLEFT, S_NEUTRAL);
		send_update(BT_NONE, DP_NEUTRAL, S_LEFT, S_NEUTRAL);
		send_update(BT_NONE, DP_NEUTRAL, S_BOTLEFT, S_NEUTRAL);
		send_update(BT_NONE, DP_NEUTRAL, S_BOTTOM, S_NEUTRAL);
		send_update(BT_NONE, DP_NEUTRAL, S_BOTRIGHT, S_NEUTRAL);
	}

	/* reset sticks position */
	pause_automation();
}


/*
 * Temporary gives back control to the user by performing controller switch.
 */
static
void
temporary_control(void)
{
	set_leds(NO_LEDS);

	/* allow the user to connect their controller back as controller 1 */
	switch_controller(VIRT_TO_REAL);

	/* wait for the user to press the button (should be on the Switch main menu) */
	wait_for_confirm();

	/* set the virtual controller as controller 1 */
	switch_controller(REAL_TO_VIRT);
}
