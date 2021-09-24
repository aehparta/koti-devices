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
#define KOTI_NRF_CHANNEL 70
#define KOTI_NRF_SPEED NRF24L01P_SPEED_250k

/* basic nrf packet parts */
#define KOTI_NRF_SIZE_HEADER 12
#define KOTI_NRF_SIZE_PAYLOAD 20
#define KOTI_NRF_SIZE (KOTI_NRF_SIZE_HEADER + KOTI_NRF_SIZE_PAYLOAD)

/* temperature and humidity, 8 bytes:
 *  4 bytes: temperature, float
 *  4 bytes: humidity, float
 */
#define KOTI_NRF_TYPE_TH 0

/* water flow in litres, 4 bytes:
 *  4 bytes: water flow, uint32_t
 */
#define KOTI_NRF_TYPE_WATER_FLOW_LITRE 1

/* water flow in millilitres, 8 bytes:
 *  8 bytes: water flow, uint64_t
 */
#define KOTI_NRF_TYPE_WATER_FLOW_MILLILITRE 2

/* simple click, 1 byte:
 *  1 byte: id of clicked entity (button etc)
 */
#define KOTI_NRF_TYPE_CLICK 2

/* nrf packet flags */
#define KOTI_NRF_FLAG_TTL_MASK 0x03
#define KOTI_NRF_FLAG_ENC_NONE 0x00
#define KOTI_NRF_FLAG_ENC_1_BLOCK 0x04
#define KOTI_NRF_FLAG_ENC_2_BLOCKS 0x08
#define KOTI_NRF_FLAG_ENC_3_BLOCKS 0x0c
#define KOTI_NRF_FLAG_ENC_BLOCKS_MASK 0x0c

/* IMPORTANT */
#pragma pack(1)

/* basic header structure */
struct koti_nrf_header {
	uint8_t mac[6]; /* sender mac address */
	/* flag bits:
	*  0-1: ttl, 2 bits
	*  2-3: 8-byte blocks of payload encrypted
	*/
	uint8_t flags;
	uint8_t seq;

	/* RC5 encryption starts here, if KOTI_NRF_FLAG_ENC_NONE is not set in flags */
	uint8_t crc;
	uint8_t rand; /* this is to make encrypted data more random ("replace" iv) */
	uint8_t bat;  /* battery charge percentage */
	uint8_t type; /* unencrypted payload crc-8 */
};

/* main packet structure */
struct koti_nrf_pck {
	struct koti_nrf_header hdr;
	/* payload */
	union {
		/* as bytes */
		uint8_t data[KOTI_NRF_SIZE_PAYLOAD];
		/* 16-bit unsigned ints */
		uint16_t *u16;
		/* 32-bit unsigned ints */
		uint32_t *u32;
		/* floats */
		float *f32;
		/* 64-bit unsigned ints */
		uint64_t *u64;
	};
};

/* IMPORTANT */
#pragma pack()

int8_t nrf24l01p_koti_init(struct spi_master *master, uint8_t ss, uint8_t ce);
void nrf24l01p_koti_quit(void);

void nrf24l01p_koti_set_key(uint8_t *key, uint8_t size);

int8_t nrf24l01p_koti_recv(struct koti_nrf_pck *pck);
int8_t nrf24l01p_koti_send(struct koti_nrf_pck *pck);

#ifdef __cplusplus
}
#endif

#endif /* _KOTI_NRF_H_ */
