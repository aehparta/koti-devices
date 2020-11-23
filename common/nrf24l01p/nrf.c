
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
#ifdef USE_AES
	uint8_t k[32];
#else
	/* on smaller platforms only RC5 is used, so key is 16 bytes at max */
	uint8_t k[16];
#endif
	memset(k, 0, sizeof(k));
	memcpy(k, key, size);
	rc5_init(&rc5, k);
#ifdef USE_AES
	/* implement */
#endif
}

int8_t nrf24l01p_koti_recv(void *p)
{
	struct koti_nrf_pck *pck = p;
	int8_t n = 0;

#ifdef USE_DRIVER_NRF24L01P
	n = nrf24l01p_recv(&nrf, pck);
	ERROR_IF(n < 0, "nrf24l01p recv failed");
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

	/* clear ttl for possible encryption iv usage */
	pck->hdr.flags &= ~KOTI_NRF_FLAG_TTL_MASK;

	/* encryption being zero is same as RC5 */
	if (!(pck->hdr.flags & KOTI_NRF_ENC_MASK)) {
		/* third block: only if second bit is set */
		if (pck->hdr.flags & KOTI_NRF_ENC_BLOCKS_3) {
			rc5_decrypt(&rc5, pck->data + 16);
			for (uint8_t i = 8; i < 16; i++) {
				/* de-apply iv */
				pck->data[i + 8] ^= pck->data[i];
			}
		}
		/* second block: always if more than first block should be encrypted */
		if (pck->hdr.flags & KOTI_NRF_ENC_BLOCKS_MASK) {
			rc5_decrypt(&rc5, pck->data + 8);
			for (uint8_t i = 0; i < 8; i++) {
				/* apply iv */
				pck->data[i + 8] ^= pck->data[i];
			}
		}
		/* first block */
		rc5_decrypt(&rc5, pck->data);
		for (uint8_t i = 0; i < 8; i++) {
			/* de-apply iv */
			pck->data[i] ^= pck->iv[i];
		}
	}

	/* calculate crc */
	uint8_t crc = crc8_dallas(pck->data, KOTI_NRF_SIZE_PAYLOAD);
	if (crc != pck->hdr.crc) {
		DEBUG_MSG("crc missmatch");
		return 0;
	}

	return n;
}

int8_t nrf24l01p_koti_send(uint8_t to, uint8_t from, void *p)
{
	struct koti_nrf_pck *pck = p;
	int8_t n = 0;

	/* set to and from */
	pck->hdr.to = to;
	pck->hdr.from = from;

	/* clear ttl for possible encryption iv usage */
	pck->hdr.flags &= ~KOTI_NRF_FLAG_TTL_MASK;

	/* set sequence number and increase it, this is mostly to make encryption iv change always */
	pck->hdr.seq = sequence++;

	/* also set random number to further randomize header as iv */
	pck->hdr.iv_rand = (uint8_t)rand();

	/* calculate crc */
	pck->hdr.crc = crc8_dallas(pck->data, KOTI_NRF_SIZE_PAYLOAD);

	/* encryption being zero is same as RC5 */
	if (!(pck->hdr.flags & KOTI_NRF_ENC_MASK)) {
		/* first block */
		for (uint8_t i = 0; i < 8; i++) {
			/* apply iv */
			pck->data[i] ^= pck->iv[i];
		}
		rc5_encrypt(&rc5, pck->data);
		/* second block: always if more than first block should be encrypted */
		if (pck->hdr.flags & KOTI_NRF_ENC_BLOCKS_MASK) {
			for (uint8_t i = 0; i < 8; i++) {
				/* apply iv */
				pck->data[i + 8] ^= pck->data[i];
			}
			rc5_encrypt(&rc5, pck->data + 8);
		}
		/* third block: only if second bit is set */
		if (pck->hdr.flags & KOTI_NRF_ENC_BLOCKS_3) {
			for (uint8_t i = 8; i < 16; i++) {
				/* apply iv */
				pck->data[i + 8] ^= pck->data[i];
			}
			rc5_encrypt(&rc5, pck->data + 16);
		}
	}
#ifdef USE_AES
	/* implement aes */
#endif

	/* set max ttl for now */
	pck->hdr.flags |= KOTI_NRF_FLAG_TTL_MASK;

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

	return n;
}
