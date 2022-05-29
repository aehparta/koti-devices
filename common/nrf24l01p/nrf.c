
#include "nrf.h"

#ifdef USE_DRIVER_NRF24L01P
struct nrf24l01p_device nrf;
#endif

/* RC5 context */
static rc5_context_t rc5;
/* sequence number for packets we send */
static uint8_t sequence;

int8_t nrf24l01p_koti_init(struct spi_master *master, uint8_t ss, uint8_t ce)
{
#ifdef USE_DRIVER_NRF24L01P
	/* nrf initialization */
	ERROR_IF_R(nrf24l01p_open(&nrf, master, ss, ce), -1, "nrf24l01+ failed to initialize");
	/* change channel, default is 70 */
	nrf24l01p_set_channel(&nrf, KOTI_NRF_CHANNEL);
	/* change speed, default is 250k */
	nrf24l01p_set_speed(&nrf, KOTI_NRF_SPEED);
#ifdef USE_KOTI_NRF_LOW_POWER
	/* set radio to the lowest setting */
	// nrf24l01p_set_tx_power(&nrf, NRF24L01P_POWER_LOW);
	/* set radio in send mode and put it in standy to save power */
	nrf24l01p_mode_tx(&nrf);
	nrf24l01p_set_standby(&nrf, true);
	nrf24l01p_set_power_down(&nrf, true);
#else
	/* set radio in listen mode */
	nrf24l01p_set_standby(&nrf, true);
	nrf24l01p_mode_rx(&nrf);
	nrf24l01p_set_standby(&nrf, false);
#endif

	DEBUG_MSG("using nrf24l01p");
#endif

#ifdef USE_BROADCAST /* lan broadcast driver for testing purposes */
	if (broadcast_init(KOTI_BROADCAST_PORT)) {
		ERROR_MSG("failed to initialize koti development broadcast on port %d", KOTI_BROADCAST_PORT);
	} else {
		DEBUG_MSG("using broadcast on port %d for development purposes", KOTI_BROADCAST_PORT);
	}
#endif

	sequence = 0;

	return 0;
}

void nrf24l01p_koti_quit(void)
{
#ifdef USE_DRIVER_NRF24L01P
	nrf24l01p_close(&nrf);
#endif
#ifdef USE_BROADCAST /* lan broadcast driver for testing purposes */
	broadcast_quit();
#endif
}

void nrf24l01p_koti_set_key(uint8_t *key, uint8_t size)
{
	/* only RC5 is used, so key is 16 bytes at max */
	uint8_t k[16];
	memset(k, 0, sizeof(k));
	memcpy(k, key, size);
	rc5_init(&rc5, k);
}

int8_t nrf24l01p_koti_recv(struct koti_nrf_pck *pck)
{
	uint8_t *p8 = (uint8_t *)pck;
	uint8_t i, crc_in;
	int8_t n = 0;

#ifdef USE_DRIVER_NRF24L01P
	n = nrf24l01p_recv(&nrf, pck);
#endif

#ifdef USE_BROADCAST /* lan broadcast driver for testing purposes */
	if (n < 1) {
		n = broadcast_recv(pck, sizeof(*pck));
	}
#endif

	/* return if no data or an error occured */
	if (n < 1) {
		return n;
	}

	// HEX_DUMP(pck, sizeof(*pck), 1);

	/* check if should decrypt */
	switch (pck->hdr.flags & KOTI_NRF_FLAG_ENC_BLOCKS_MASK) {
	case KOTI_NRF_FLAG_ENC_RC5_4_BLOCKS:
		/* fourth data block */
		rc5_decrypt(&rc5, p8 + 24);
		for (i = 16; i < 24; i++) {
			p8[i + 8] ^= p8[i];
		}
	case KOTI_NRF_FLAG_ENC_RC5_3_BLOCKS:
		/* third data block */
		rc5_decrypt(&rc5, p8 + 16);
		for (i = 8; i < 16; i++) {
			p8[i + 8] ^= p8[i];
		}
	case KOTI_NRF_FLAG_ENC_RC5_2_BLOCKS:
		/* second data block */
		rc5_decrypt(&rc5, p8 + 8);
	case KOTI_NRF_FLAG_ENC_RC5_1_BLOCK:
		/* first data block */
		rc5_decrypt(&rc5, p8 + 4);
	}

	/* calculate crc */
	crc_in = pck->hdr.crc;
	pck->hdr.crc = 0;
	pck->hdr.flags = pck->hdr.rand;
	if (crc8_dallas(p8, KOTI_NRF_SIZE) != crc_in) {
		DEBUG_MSG("crc missmatch");
		return 0;
	}

	return 32;
}

int8_t nrf24l01p_koti_send(struct koti_nrf_pck *pck)
{
	uint8_t *p8 = (uint8_t *)pck;
	uint8_t i, enc, flags;

	/* save flags temporarily and set max ttl for now */
	flags = pck->hdr.flags | KOTI_NRF_FLAG_TTL_MASK;

	/* set sequence number and increase it */
	pck->hdr.seq = sequence++;
	/* NOTE: this is a compatibility hack to make old Kotivo system to route these packets */
	// if (enc < KOTI_NRF_FLAG_ENC_3_BLOCKS) {
	// 	p8[31] = sequence;
	// }

	/* make encrypted data more random  */
	pck->hdr.rand = (uint8_t)rand();
	pck->hdr.reserved_rand = (uint8_t)rand();
	pck->hdr.flags = pck->hdr.rand;
	/* calculate crc */
	pck->hdr.crc = 0;
	pck->hdr.crc = crc8_dallas(p8, KOTI_NRF_SIZE);

	/* check if packet should be encrypted */
	enc = pck->hdr.flags & KOTI_NRF_FLAG_ENC_BLOCKS_MASK;
	if (enc >= KOTI_NRF_FLAG_ENC_RC5_1_BLOCK) {
		rc5_encrypt(&rc5, p8 + 4);
		/* second data block */
		if (enc >= KOTI_NRF_FLAG_ENC_RC5_2_BLOCKS) {
			rc5_encrypt(&rc5, p8 + 8);
			/* third data block */
			if (enc >= KOTI_NRF_FLAG_ENC_RC5_3_BLOCKS) {
				for (i = 8; i < 16; i++) {
					p8[i + 8] ^= p8[i];
				}
				rc5_encrypt(&rc5, p8 + 16);
				/* fourth data block */
				if (enc >= KOTI_NRF_FLAG_ENC_RC5_4_BLOCKS) {
					for (i = 16; i < 24; i++) {
						p8[i + 8] ^= p8[i];
					}
					rc5_encrypt(&rc5, p8 + 24);
				}
			}
		}
	}

	/* resume original flags */
	pck->hdr.flags = flags;

#ifdef USE_DRIVER_NRF24L01P
#ifdef USE_KOTI_NRF_LOW_POWER
	/* switch power on */
	nrf24l01p_set_power_down(&nrf, false);
	/* enable radio */
	nrf24l01p_set_standby(&nrf, false);
	/* write tx buffer */
	nrf24l01p_tx_wr(&nrf, pck);
	/* disable radio */
	nrf24l01p_set_standby(&nrf, true);
	/* power down */
	nrf24l01p_set_power_down(&nrf, true);
#else
	/* use nrf24l01p_send() which automatically keeps radio on and switches back to listen mode */
	nrf24l01p_send(&nrf, pck);
#endif
#endif

#ifdef USE_BROADCAST /* lan broadcast driver for testing purposes */
	broadcast_send(pck, sizeof(*pck));
#endif

	return 32;
}
