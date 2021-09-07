/*
 * Generic switch(es).
 */

#include <koti.h>
#include "web.h"

// struct spi_master master;

int p_init(int argc, char *argv[])
{
	/* very low level platform initialization */
	os_init();
	/* debug/log init */
	log_init();

	/* init development web interface (or do nothing if target does not use it) */
	CRIT_IF_R(web_init(), -1, "failed to initialize web interface");

/* initialize spi master */
// #ifdef USE_DRIVER_NRF24L01P /* this is to disable real nrf driver for development purposes */
// #ifdef USE_FTDI
// 	/* open ft232h type device and try to see if it has a nrf24l01+ connected to it through mpsse-spi */
// 	ERROR_IF_R(os_ftdi_use(OS_FTDI_GPIO_0_TO_63, 0, 0, NULL, NULL), -1, "unable to open ftdi device for gpio 0-63");
// 	os_ftdi_set_mpsse(SPI_SCLK);
// #endif
// 	ERROR_IF_R(spi_master_open(
// 	               &master,     /* must give pre-allocated spi master as pointer */
// 	               SPI_CONTEXT, /* context depends on platform */
// 	               SPI_FREQUENCY,
// 	               SPI_MISO,
// 	               SPI_MOSI,
// 	               SPI_SCLK),
// 	           -1, "failed to open spi master");
// #endif

// 	/* nrf initialization */
#if defined(USE_SPI) && defined(USE_DRIVER_NRF24L01P)
	ERROR_IF_R(nrf24l01p_koti_init(&master, NRF_SS, NRF_CE), -1, "nrf24l01+ failed to initialize");
#elif defined(USE_BROADCAST)
	ERROR_IF_R(nrf24l01p_koti_init(NULL, 0, 0), -1, "nrf24l01+ failed to initialize");
#else
	#error "no communication driver available"
#endif
	nrf24l01p_koti_set_key((uint8_t *)"12345678", 8);

	return 0;
}

void p_exit(int retval)
{
	nrf24l01p_koti_quit();
#ifdef USE_SPI
	spi_master_close(&master);
#endif
	web_quit();
	log_quit();
	os_quit();
	exit(retval);
}

#ifdef TARGET_ESP32
int app_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	/* init */
	if (p_init(argc, argv)) {
		ERROR_MSG("initialization failed");
		p_exit(EXIT_FAILURE);
	}

	/* program loop */
	DEBUG_MSG("starting main program loop");
	while (1) {
		struct koti_nrf_pck pck;
		os_wdt_reset();

		/* lets not waste all cpu */
		os_sleepf(2);
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}
