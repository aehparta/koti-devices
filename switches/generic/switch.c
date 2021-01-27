/*
 * Switches.
 */

#include <koti.h>
#include "switch.h"

static bool switches[SWITCH_COUNT];

int8_t switch_init(void)
{
	for (int8_t i = 0; i < SWITCH_COUNT; i++) {
		switches[i] = false;
	}

	return 0;
}

int8_t switch_toggle(uint8_t sw)
{
	if (sw >= SWITCH_COUNT) {
		return -1;
	}
	switches[sw] = !switches[sw];
	return switches[sw];
}
