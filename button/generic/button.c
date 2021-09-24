/*
 * Switches.
 */

#include <koti.h>
#include "button.h"

static bool buttons[BUTTON_COUNT];
static const uint8_t mac[6] = {0, 1, 2, 3, 4, 5};

static void button_send_state(uint8_t btn);

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

bool button_state(uint8_t btn)
{
	if (btn >= BUTTON_COUNT) {
		return false;
	}
	return buttons[btn];
}

bool button_on(uint8_t btn)
{
	if (btn >= BUTTON_COUNT) {
		return false;
	}
	buttons[btn] = true;
	button_send_state(btn);
	return true;
}

bool button_off(uint8_t btn)
{
	if (btn >= BUTTON_COUNT) {
		return false;
	}
	buttons[btn] = false;
	button_send_state(btn);
	return false;
}

bool button_toggle(uint8_t btn)
{
	if (btn >= BUTTON_COUNT) {
		return false;
	}
	return buttons[btn] ? button_off(btn) : button_on(btn);
}

void button_send_state(uint8_t btn)
{
	struct koti_nrf_pck pck;
	memcpy(pck.hdr.mac, mac, 6);

	pck.hdr.flags = KOTI_NRF_FLAG_ENC_1_BLOCK;
	pck.hdr.bat = 0;
	pck.hdr.type = KOTI_NRF_TYPE_CLICK;
	memset(pck.data, 0, sizeof(pck.data));
	pck.data[0] = btn;
	nrf24l01p_koti_send(&pck);
}