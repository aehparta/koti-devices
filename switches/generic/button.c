/*
 * Switches.
 */

#include <koti.h>
#include "button.h"

static bool buttons[BUTTON_COUNT];

int8_t button_init(void)
{
	for (int8_t i = 0; i < BUTTON_COUNT; i++) {
		buttons[i] = false;
	}

	return 0;
}

uint8_t button_count(void)
{
	return BUTTON_COUNT;
}

bool button_state(uint8_t sw)
{
	if (sw >= BUTTON_COUNT) {
		return false;
	}
	return buttons[sw];
}

bool button_on(uint8_t sw)
{
	if (sw >= BUTTON_COUNT) {
		return false;
	}
	if (!buttons[sw]) {
	}
	buttons[sw] = true;
	return true;
}

bool button_off(uint8_t sw)
{
	if (sw >= BUTTON_COUNT) {
		return false;
	}
	buttons[sw] = false;
	return false;
}

bool button_toggle(uint8_t sw)
{
	if (sw >= BUTTON_COUNT) {
		return false;
	}
	return buttons[sw] ? button_off(sw) : button_on(sw);
}
