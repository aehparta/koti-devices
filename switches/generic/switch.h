/*
 * Switches.
 */

#ifndef _SWITCH_H_
#define _SWITCH_H_

#include <stdint.h>
#include <stdbool.h>

#define SWITCH_COUNT 8

int8_t switch_init(void);
#define switch_quit()

bool switch_state(uint8_t sw);
bool switch_on(uint8_t sw);
bool switch_off(uint8_t sw);
bool switch_toggle(uint8_t sw);

#endif /* _SWITCH_H_ */
