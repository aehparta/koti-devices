/*
 * NRF24l01+ packet definitions.
 */

#ifndef _KOTI_H_
#define _KOTI_H_

#include <libe/libe.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KOTI_BROADCAST_PORT     17117

#ifdef TARGET_LINUX
#include "opt.h"
#endif
#include "nrf24l01p/nrf.h"

#ifdef __cplusplus
}
#endif

#endif /* _KOTI_H_ */
