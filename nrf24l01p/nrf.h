/*
 * NRF24l01+ packet definitions.
 */

#ifndef _KOTI_NRF_H_
#define _KOTI_NRF_H_

#include <koti.h>

#ifdef __cplusplus
extern "C" {
#endif

/* basic nrf packet parts */
#define KOTI_NRF_SIZE_HEADER            8
#define KOTI_NRF_SIZE_PAYLOAD           24
#define KOTI_NRF_SIZE                   (KOTI_NRF_SIZE_HEADER + KOTI_NRF_SIZE_PAYLOAD)


/* broadcast ID */
#define KOTI_NRF_ID_BROADCAST           0xff
/* sender id for a data packet that contains uuid */
#define KOTI_NRF_ID_UUID                0xfe


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

/* encryption */
#define KOTI_NRF_ENC_NONE               0x00
#define KOTI_NRF_ENC_AES128             0x20
#define KOTI_NRF_ENC_AES256             0x40
#define KOTI_NRF_ENC_RES1               0x60
#define KOTI_NRF_ENC_RES2               0x80
/* Small target encryptions, need to do performance analysis compared to power usage in PIC16 and AVR.
 * Security-wise they are similar enough in my opinion. Still, RC5 is the least unsecure and most tested?
 */
#define KOTI_NRF_ENC_RC5                0xa0 /* RC5 is the strongest candidate for small target encryption */
#define KOTI_NRF_ENC_XTEA               0xc0 /* or maybe XTEA */
#define KOTI_NRF_ENC_XXTEA              0xe0 /* XXTEA is probably the next one after RC5 */
#define KOTI_NRF_ENC_MASK               0xe0


/* IMPORTANT */
#pragma pack(1)

/* basic header structure */
struct koti_nrf_header {
	/* flags, bits:
	 *  0-1: ttl, 2 bits
	 *  2: acknowledge
	 */
	uint8_t flags;
	uint8_t dst; /* receiver id */
	uint8_t src; /* sender id */
	uint8_t type; /* packet type */

	/* encryption, bits:
	 *  0-4: bytes of payload encrypted
	 *  5-7: encryption used, 3 bits
	 */
	uint8_t enc;
	uint8_t crc; /* unencrypted payload crc-8 */

	uint8_t seq; /* sender specific sequence number */
	uint8_t x7;
};

/* payload structures */

struct koti_nrf_time {
	uint32_t timestamp; /* unix timestamp */
	int32_t timezone; /* timezone offset in seconds */
	int32_t latitude; /* latitude in seconds */
	int32_t longitude; /* longitude in seconds */
};


/* main packet structure */
struct koti_nrf {
	/* header */
	union {
		struct koti_nrf_header header;
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
struct koti_nrf_broadcast_uuid {
	/* basic header structure */
	struct koti_nrf_header header;
	/* uuid */
	uint8_t uuid[16];
	/* payload */
	union {
		/* as bytes */
		uint8_t data[8];
		/* two floats */
		float f32[2];
	};
};


/* IMPORTANT */
#pragma pack()


int8_t nrf24l01p_koti_init(struct nrf24l01p_device *nrf, struct spi_master *master, uint8_t ss, uint8_t ce);

int8_t nrf24l01p_koti_recv(struct nrf24l01p_device *nrf, void *koti_nrf_packet);
int8_t nrf24l01p_koti_send(struct nrf24l01p_device *nrf, void *koti_nrf_packet);


#ifdef __cplusplus
}
#endif

#endif /* _KOTI_NRF_H_ */
