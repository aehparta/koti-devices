/*
 * Buttons.
 */

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdint.h>
#include <stdbool.h>

int8_t button_init(void);
#define button_quit()

uint8_t button_count(void);

bool button_state(uint8_t sw);
bool button_on(uint8_t sw);
bool button_off(uint8_t sw);
bool button_toggle(uint8_t sw);

#endif /* _BUTTON_H_ */
