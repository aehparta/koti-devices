/*
 * NRF24l01+ packet definitions.
 */

#ifndef _KOTI_H_
#define _KOTI_H_

#include <libe/libe.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* packet types */

/* id query */
#define KOTI_TYPE_ID_QUERY 1

/* power supply */
#define KOTI_TYPE_PSU 2

/* water flow in litres, 8 bytes, uint64_t */
#define KOTI_TYPE_WATER_FLOW_LITRE 3

/* water flow in millilitres, 8 bytes, uint64_t */
#define KOTI_TYPE_WATER_FLOW_MILLILITRE 4

/* temperature and humidity, 8 bytes:
 *  4 bytes: temperature, float
 *  4 bytes: humidity, float
 */
#define KOTI_TYPE_TH 5

/* extremely simple click, no data, just sent when click occurs */
#define KOTI_TYPE_CLICK 6

/* count, 8 bytes, uint64_t */
#define KOTI_TYPE_COUNT 7

/* debug data packet */
#define KOTI_TYPE_DEBUG 255


/******************************************************************************/
/* power supply */
#define KOTI_PSU_UNKNOWN 0
#define KOTI_PSU_MAINS_GENERIC 1
#define KOTI_PSU_BATTERY_LITHIUM 0x41
#define KOTI_PSU_BATTERY_ALKALINE 0x42
#define KOTI_PSU_BATTERY_RECHARGEABLE_LEAD 0x81
#define KOTI_PSU_BATTERY_RECHARGEABLE_NICD 0x82
#define KOTI_PSU_BATTERY_RECHARGEABLE_NIMH 0x83


/******************************************************************************/
#define KOTI_BROADCAST_PORT 21579

#ifdef TARGET_LINUX
#include "opt.h"
#endif
#include "nrf24l01p/nrf.h"

#ifdef USE_HTTPD
#include "httpd/httpd.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* _KOTI_H_ */
