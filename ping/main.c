/*
 * nrf24l01+ example ping
 *
 * Run nrf24l01-listen on some other device to get responses to ping packets.
 *
 * NOTE: This example will not run properly on all platforms, example avr,
 * since os_timef() cannot be properly implemented in those smaller platforms
 */

#include <koti.h>


struct spi_master master;


int p_init(int argc, char *argv[])
{
	/* very low level platform initialization */
	os_init();
	/* debug/log init */
	log_init();

	/* initialize spi master */
	ERROR_IF_R(spi_master_open(&master, ((void *)SPI_CONTEXT), SPI_FREQUENCY, SPI_MISO, SPI_MOSI, SPI_SCLK), -1, "failed to open spi master");

	/* nrf initialization, hardcoded pins so zeroes there */
	nrf24l01p_koti_init(&master, NRF_SS, NRF_CE);
	nrf24l01p_koti_set_key((uint8_t *)"12345678", 8);

	return 0;
}

void p_exit(int retval)
{
	nrf24l01p_koti_quit();
	spi_master_close(&master);
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
	os_time_t t = os_timef();

	/* init */
	if (p_init(argc, argv)) {
		ERROR_MSG("initialization failed");
		os_sleepi(1);
		p_exit(EXIT_FAILURE);
	}

	/* program loop */
	INFO_MSG("starting main program loop");
	while (1) {
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}
