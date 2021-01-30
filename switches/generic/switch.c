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

bool switch_state(uint8_t sw)
{
	if (sw >= SWITCH_COUNT) {
		return false;
	}
	return switches[sw];
}

bool switch_on(uint8_t sw)
{
	if (sw >= SWITCH_COUNT) {
		return false;
	}
	switches[sw] = true;
	return true;
}

bool switch_off(uint8_t sw)
{
	if (sw >= SWITCH_COUNT) {
		return false;
	}
	switches[sw] = false;
	return false;
}

bool switch_toggle(uint8_t sw)
{
	if (sw >= SWITCH_COUNT) {
		return false;
	}
	return switches[sw] ? switch_off(sw) : switch_on(sw);
}
