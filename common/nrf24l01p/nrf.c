
#include "nrf.h"


#ifdef USE_DRIVER_NRF24L01P
struct nrf24l01p_device nrf;
#endif


int8_t nrf24l01p_koti_init(struct spi_master *master, uint8_t ss, uint8_t ce)
{
#ifdef USE_DRIVER_NRF24L01P
	/* nrf initialization */
	ERROR_IF_R(nrf24l01p_open(&nrf, master, ss, ce), -1, "nrf24l01+ failed to initialize");
	/* change channel, default is 70 */
	nrf24l01p_set_channel(&nrf, KOTI_NRF_CHANNEL);
	/* change speed, default is 250k */
	nrf24l01p_set_speed(&nrf, KOTI_NRF_SPEED);
	/* set radio in listen mode */
	nrf24l01p_flush_rx(&nrf);
	/* enable radio */
	// nrf24l01p_set_standby(&nrf, false);
	DEBUG_MSG("using nrf24l01p");
#endif

#ifdef USE_BROADCAST
	if (broadcast_init(KOTI_BROADCAST_PORT)) {
		ERROR_MSG("failed to initialize koti development broadcast on port %d", KOTI_BROADCAST_PORT);
	} else {
		DEBUG_MSG("using broadcast on port %d for development purposes", KOTI_BROADCAST_PORT);
	}
#endif

	return 0;
}

void nrf24l01p_koti_quit(void)
{
#ifdef USE_DRIVER_NRF24L01P
	nrf24l01p_close(&nrf);
#endif
#ifdef USE_BROADCAST
	broadcast_quit();
#endif
}

int8_t nrf24l01p_koti_recv(void *pck)
{
#ifdef USE_DRIVER_NRF24L01P
	int8_t n = nrf24l01p_recv(&nrf, pck);
	if (n > 0) {
		return n;
	}
	// ERROR_IF(n < 0, "nrf24l01p recv failed");
#endif

#ifdef USE_BROADCAST
	return broadcast_recv(pck, sizeof(struct koti_nrf_pck));
#endif

	return 0;
}

int8_t nrf24l01p_koti_send(void *pck)
{
	int8_t n = 0;

#ifdef USE_DRIVER_NRF24L01P
#endif

#ifdef USE_BROADCAST
	broadcast_send(pck, sizeof(struct koti_nrf_pck));
#endif

	return n;
}
