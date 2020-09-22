/*
 * nrf24l01+ example listener
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
#ifdef USE_DRIVER_NRF24L01P /* this is to disable real nrf driver for development purposes */
#ifdef USE_FTDI
	/* open ft232h type device and try to see if it has a nrf24l01+ connected to it through mpsse-spi */
	ERROR_IF_R(os_ftdi_use(OS_FTDI_GPIO_0_TO_63, 0, 0, NULL, NULL), -1, "unable to open ftdi device for gpio 0-63");
	os_ftdi_set_mpsse(SPI_SCLK);
#endif
	ERROR_IF_R(spi_master_open(
	               &master, /* must give pre-allocated spi master as pointer */
	               SPI_CONTEXT, /* context depends on platform */
	               SPI_FREQUENCY,
	               SPI_MISO,
	               SPI_MOSI,
	               SPI_SCLK
	           ), -1, "failed to open spi master");
#endif

	/* nrf initialization */
	ERROR_IF_R(nrf24l01p_koti_init(&master, NRF_SS, NRF_CE), -1, "nrf24l01+ failed to initialize");
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
	/* init */
	if (p_init(argc, argv)) {
		ERROR_MSG("initialization failed");
		os_delay_ms(500); /* some hardware (pic8) might not print the last line feed without this */
		p_exit(EXIT_FAILURE);
	}

	/* program loop */
	INFO_MSG("starting main program loop");
	while (1) {
		struct koti_nrf_pck pck;

		os_wdt_reset();

		memset(&pck, 0, sizeof(pck));
		memcpy(pck.data, "123456789abcdef", 16);
		pck.hdr.flags |= KOTI_NRF_ENC_BLOCKS_2;

		DEBUG_MSG("sending packet");
		nrf24l01p_koti_send(KOTI_NRF_ID_BRIDGE, KOTI_NRF_ID_UUID, &pck);
		HEX_DUMP(&pck, sizeof(pck), 1);
		ASCII_DUMP(&pck, sizeof(pck), 0);

		/* lets not waste all cpu */
		os_sleepf(2);
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}
