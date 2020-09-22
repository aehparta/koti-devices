/*
 * NRF24l01+ packet definitions.
 */

#ifndef _KOTI_NRF_H_
#define _KOTI_NRF_H_

#include <koti.h>

#ifdef __cplusplus
extern "C" {
#endif

/* nrf24l01p settings */
#define KOTI_NRF_CHANNEL                17
#define KOTI_NRF_SPEED                  NRF24L01P_SPEED_2M

/* basic nrf packet parts */
#define KOTI_NRF_SIZE_HEADER            8
#define KOTI_NRF_SIZE_PAYLOAD           24
#define KOTI_NRF_SIZE                   (KOTI_NRF_SIZE_HEADER + KOTI_NRF_SIZE_PAYLOAD)


/* broadcast ID */
#define KOTI_NRF_ID_BROADCAST           0x00
/* used to send packets to bridges only */
#define KOTI_NRF_ID_BRIDGE              0x01
/* sender id for a data packet that contains uuid (device has no valid id or does not even use one) */
#define KOTI_NRF_ID_UUID                0x02

/* nrf packet types (0-127) */
#define KOTI_NRF_HELLO                  0
#define KOTI_NRF_GET_ID                 1
#define KOTI_NRF_SET_ID                 2

/* Temperature and humidity, 8 bytes:
 *  4 bytes: temperature, float
 *  4 bytes: humidity, float
 */
#define KOTI_NRF_TH                     3

/* Pressure and windspeed, 8 bytes:
 *  4 bytes: pressure, float
 *  4 bytes: windspeed, float
 */
#define KOTI_NRF_PRESSURE_WINDSPEED     4

/* Water flow, 4 bytes:
 *  4 bytes: water flow, float
 */
#define KOTI_NRF_WATER_FLOW             5


/* nrf packet flags */
#define KOTI_NRF_FLAG_TTL_MASK          0x03

/* ack flag */
#define KOTI_NRF_ACK_BIT                0x04

/* encryption */
/* RC5 is for small target encryptions.
 * Security-wise it is quite well tested and pretty much enough for most IOT stuff.
 * (should do performance analysis compared to power usage in 8-bit PICs and AVR)
 */
#define KOTI_NRF_ENC_RC5                0x00
#define KOTI_NRF_ENC_AES128             0x40
#define KOTI_NRF_ENC_AES256             0x80
#define KOTI_NRF_ENC_NONE               0xc0
#define KOTI_NRF_ENC_MASK               0xc0

#define KOTI_NRF_ENC_BLOCKS_1           0x00
#define KOTI_NRF_ENC_BLOCKS_2           0x10
#define KOTI_NRF_ENC_BLOCKS_3           0x20
#define KOTI_NRF_ENC_BLOCKS_MASK        0x30


/* IMPORTANT */
#pragma pack(1)

/* basic header structure */
struct koti_nrf_header {
	union {
		struct {
			/* receiver id */
			uint8_t to;
			/* sender id */
			uint8_t from;
			/* sender specific incremental sequence number */
			uint8_t seq;
			/* this is to make header use as iv better and also to make id more unique */
			uint8_t iv_rand;
		};
		/* unique packet id used for forwarding single packet only once, consist all fields from struct above */
		uint32_t id;
	};

	/* flags, bits:
	 *  0-1: ttl, 2 bits (must be zero when using header as iv)
	 *  2: acknowledge
	 *  4-5: 8-byte blocks of payload encrypted (0: first block only, 1: first 2 blocks, 2 or 3: all of it)
	 *  6-7: encryption used
	 */
	uint8_t flags;

	/* packet type */
	uint8_t type;

	/* reserved */
	uint8_t res1;

	/* unencrypted payload crc-8 */
	uint8_t crc;
};

/* payload structures */

struct koti_nrf_time {
	uint32_t timestamp; /* unix timestamp */
	int32_t timezone; /* timezone offset in seconds */
	int32_t latitude; /* latitude in seconds */
	int32_t longitude; /* longitude in seconds */
};


/* main packet structure */
struct koti_nrf_pck {
	/* header */
	union {
		struct koti_nrf_header hdr;
		/* header as IV (initialization vector) */
		uint8_t iv[8];
	};
	/* payload */
	union {
		/* as bytes */
		uint8_t data[KOTI_NRF_SIZE_PAYLOAD];
		/* six floats */
		float f32[6];
		/* time */
		struct koti_nrf_time time;
	};
};


/* one way packet structure with embedded uuid */
struct koti_nrf_pck_broadcast_uuid {
	/* header */
	union {
		struct koti_nrf_header hdr;
		/* header as IV (initialization vector) */
		uint8_t iv[8];
	};
	/* payload */
	union {
		/* as bytes */
		uint8_t data[8];
		/* two floats */
		float f32[2];
	};
	/* uuid */
	uint8_t uuid[16];
};


/* IMPORTANT */
#pragma pack()


int8_t nrf24l01p_koti_init(struct spi_master *master, uint8_t ss, uint8_t ce);
void nrf24l01p_koti_quit(void);

void nrf24l01p_koti_set_key(uint8_t *key, uint8_t size);

int8_t nrf24l01p_koti_recv(void *pck);
int8_t nrf24l01p_koti_send(uint8_t to, uint8_t from, void *pck);


#ifdef __cplusplus
}
#endif

#endif /* _KOTI_NRF_H_ */
