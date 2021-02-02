/*
 * Switches.
 */

#include <koti.h>
#include "button.h"
#include "device-uuid.h"

static bool buttons[BUTTON_COUNT];
static const uint8_t uuid[] = UUID_ARRAY;

static void button_send_states(void);

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
	button_send_states();
	return true;
}

bool button_off(uint8_t sw)
{
	if (sw >= BUTTON_COUNT) {
		return false;
	}
	buttons[sw] = false;
	button_send_states();
	return false;
}

bool button_toggle(uint8_t sw)
{
	if (sw >= BUTTON_COUNT) {
		return false;
	}
	return buttons[sw] ? button_off(sw) : button_on(sw);
}

void button_send_states(void)
{
	struct koti_nrf_pck_broadcast_uuid pck;
	memcpy(pck.uuid, uuid, 16);
	
	pck.hdr.flags = 0;
	pck.hdr.bat = 0;
	pck.hdr.type = KOTI_NRF_TYPE_BUTTONS;
	memset(pck.data, 0, sizeof(pck.data));
	pck.data[0] = BUTTON_COUNT;
	for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
		pck.data[1 + (i >> 3)] |= buttons[i] ? (1 << (i & 0x07)) : 0;
	}
	nrf24l01p_koti_send(KOTI_NRF_ID_BRIDGE, KOTI_NRF_ID_UUID, &pck);
}