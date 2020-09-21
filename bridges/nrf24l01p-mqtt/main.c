/*
 * nrf24l01+ example listener
 */

#include <koti.h>


/* command line options */
struct opt_option opt_all[] = {
	{ 'h', "help", no_argument, 0, NULL, NULL, "display this help and exit", { 0 } },

	/* influxdb server */
	{
		'i', "influxdb-ip", required_argument, 0, "127.0.0.1", NULL,
		"remote InfluxDB server host IP address\n"
		"(hostname not accepted, I was too lazy to implement resolve here)\n"
		"this is for sending data through UDP, see InfluxDB UDP support for more"
		, { OPT_FILTER_INT, 1, 65535 }
	},
	{ 'p', "influxdb-port", required_argument, 0, "8089", NULL, "remote InfluxDB UDP receive port", { OPT_FILTER_INT, 1, 65535 } },

	/* http server */
	{ 'P', "http-port", required_argument, 0, "80", NULL, "internal http daemon port", { OPT_FILTER_INT, 1, 65535 } },
	{ 'D', "html", required_argument, 0, "./web", NULL, "internal http daemon public html directory", { 0 } },

	{ 0, 0, 0, 0, 0, 0, 0, { 0 } }
};

/* global for spi master */
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
	/* parse options */
	IF_R(opt_init(opt_all, NULL, NULL, NULL) || opt_parse(argc, argv), EXIT_FAILURE);

	/* init */
	if (p_init(argc, argv)) {
		ERROR_MSG("initialization failed");
		os_delay_ms(500); /* some hardware (pic8) might not print the last line feed without this */
		p_exit(EXIT_FAILURE);
	}

	/* program loop */
	INFO_MSG("starting main program loop");
	while (1) {
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
			DEBUG_MSG("got packet");
			HEX_DUMP(&pck, sizeof(pck), 1);
		}

		/* lets not waste all cpu */
		os_sleepf(0.001);
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}
