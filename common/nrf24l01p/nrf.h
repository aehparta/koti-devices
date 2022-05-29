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
#define KOTI_NRF_SIZE_HEADER 8
#define KOTI_NRF_SIZE_PAYLOAD 24
#define KOTI_NRF_SIZE (KOTI_NRF_SIZE_HEADER + KOTI_NRF_SIZE_PAYLOAD)

/* pre-defined addresses */
#define KOTI_NRF_ADDR_CTRL 0x00
#define KOTI_NRF_ADDR_BROADCAST 0xff

/* battery percentage */
#define KOTI_NRF_TYPE_BATTERY 0

/* water flow in litres, 4 bytes:
 *  4 bytes: water flow, uint32_t
 */
#define KOTI_NRF_TYPE_WATER_FLOW_LITRE 1

/* water flow in millilitres, 8 bytes:
 *  8 bytes: water flow, uint64_t
 */
#define KOTI_NRF_TYPE_WATER_FLOW_MILLILITRE 2

/* temperature and humidity, 8 bytes:
 *  4 bytes: temperature, float
 *  4 bytes: humidity, float
 */
#define KOTI_NRF_TYPE_TH 3

/* simple click, 1 byte:
 *  1 byte: id of clicked entity (button etc)
 */
#define KOTI_NRF_TYPE_CLICK 4

/* nrf packet flags */
#define KOTI_NRF_FLAG_TTL_MASK 0x03
#define KOTI_NRF_FLAG_ENC_NONE 0x00
#define KOTI_NRF_FLAG_ENC_BLOCKS_MASK 0xe0
#define KOTI_NRF_FLAG_ENC_RC5_1_BLOCK 0x20
#define KOTI_NRF_FLAG_ENC_RC5_2_BLOCKS 0x40
#define KOTI_NRF_FLAG_ENC_RC5_3_BLOCKS 0x60
#define KOTI_NRF_FLAG_ENC_RC5_4_BLOCKS 0x80

/* IMPORTANT */
#pragma pack(1)

/* basic header structure */
struct koti_nrf_header {
	uint8_t to;   /* to address */
	uint8_t from; /* from address */
	/* flag bits:
	 *  0-1: ttl, 2 bits
	 *  5-7: 8-byte blocks of encrypted parts
	 */
	uint8_t flags;
	uint8_t seq;

	/* encryption starts here (if KOTI_NRF_FLAG_ENC_NONE is not set in flags) */
	uint8_t crc;  /* crc of whole packet unencrypted with flags set to rand */
	uint8_t rand; /* this is to make encrypted data more random ("replace" iv) */
	uint8_t type;
	uint8_t reserved_rand; /* might be used for something in future, random for now */
};

/* main packet structure */
struct koti_nrf_pck {
	struct koti_nrf_header hdr;
	/* payload */
	union {
		/* as bytes */
		uint8_t data[KOTI_NRF_SIZE_PAYLOAD];
		/* 16-bit unsigned ints */
		uint16_t u16[12];
		/* 32-bit unsigned ints */
		uint32_t u32[6];
		/* floats */
		float f32[6];
		/* 64-bit unsigned ints */
		uint64_t u64[3];

		/* uuid */
		struct {
			uint8_t uuid_padding[8];
			uint8_t uuid[16];
		};
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
