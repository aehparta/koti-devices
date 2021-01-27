/*
 * Switches.
 */

#ifndef _SWITCH_H_
#define _SWITCH_H_

#include <stdint.h>

#define SWITCH_COUNT 8

int8_t switch_init(void);
#define switch_quit()

int8_t switch_toggle(uint8_t sw);

#endif /* _SWITCH_H_ */
