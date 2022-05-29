/*
 * nrf24l01+ example listener
 */

#include <koti.h>
#include "mqtt.h"

/* command line options */
struct opt_option opt_all[] = {
    {0, "help", no_argument, 0, NULL, NULL, "display this help and exit", {0}},

    /* mqtt server */
    {'H', "mqtt-host", required_argument, 0, "localhost", NULL, "mqtt server hostname", {0}},
    {'P', "mqtt-port", required_argument, 0, "1883", NULL, "mqtt server port", {OPT_FILTER_INT, 1, 65535}},
    {'X', "mqtt-prefix", required_argument, 0, NULL, NULL, "mqtt topic prefix", {0}},

    {0, 0, 0, 0, 0, 0, 0, {0}}};

/* global for spi master */
struct spi_master master;

int p_init(int argc, char *argv[])
{
	/* low level platform initialization */
	os_init();
	/* debug/log init */
	log_init();

	/* initialize spi master */
#ifdef USE_SPI /* this is to disable real nrf driver for development purposes */
#ifdef USE_FTDI
	/* open ft232h type device and try to see if it has a nrf24l01+ connected to it through mpsse-spi */
	ERROR_IF_R(os_ftdi_use(OS_FTDI_GPIO_0_TO_63, 0x0403, 0x6014, NULL, NULL), -1, "unable to open ftdi device for gpio 0-63");
	os_ftdi_set_mpsse(SPI_SCLK);
#endif
	ERROR_IF_R(spi_master_open(
	               &master,     /* must give pre-allocated spi master as pointer */
	               SPI_CONTEXT, /* context depends on platform */
	               SPI_FREQUENCY,
	               SPI_MISO,
	               SPI_MOSI,
	               SPI_SCLK),
	           -1, "failed to open spi master");
#endif

	/* nrf initialization */
	ERROR_IF_R(nrf24l01p_koti_init(&master, NRF_SS, NRF_CE), -1, "nrf24l01+ failed to initialize");
	nrf24l01p_koti_set_key((uint8_t *)"12345678", 8);

	/* mqtt initialization and connect */
	ERROR_IF_R(mqtt_init(), -1, "mqtt initialization failed");
	ERROR_IF_R(mqtt_connect(opt_get('H'), opt_get_int('P')), -1, "mqtt connect to server failed");

	return 0;
}

void p_exit(int retval)
{
	nrf24l01p_koti_quit();
#ifdef USE_SPI
	spi_master_close(&master);
#endif
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
	/* parse options */
	IF_R(opt_init(opt_all, NULL, NULL, NULL) || opt_parse(argc, argv), EXIT_FAILURE);

	/* init */
	if (p_init(argc, argv)) {
		ERROR_MSG("initialization failed");
		p_exit(EXIT_FAILURE);
	}

	/* program loop */
	while (1) {
		static uint64_t last = 0;
		int ok;
		struct koti_nrf_pck pck;

		os_wdt_reset();

		memset(&pck, 0, sizeof(pck));
		ok = nrf24l01p_koti_recv(&pck);
		if (ok < 0) {
			CRIT_MSG("device disconnected?");
			os_delay_ms(1000);
			break;
		} else if (ok > 0) {
			DEBUG_MSG("got packet (battery: %d%%) (u32: %u, u64: %lu)",
			          pck.hdr.bat,
			          pck.u32[0], pck.u64[0]);
			last = pck.u64[0];
			HEX_DUMP(&pck, sizeof(pck), 1);
			ASCII_DUMP(&pck, sizeof(pck), 0);
		}

		/* lets not waste all cpu */
		os_sleepf(0.001);
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}
