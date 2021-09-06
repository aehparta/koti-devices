/*
 * nRF24L01+ proxy
 */

#include <libe/libe.h>

struct spi_master master;
struct nrf24l01p_device nrf;

void p_exit(int return_code)
{
	static int c = 0;
	c++;
	if (c > 1) {
		exit(return_code);
	}
	broadcast_quit();
	nrf24l01p_close(&nrf);
	log_quit();
	exit(return_code);
}

void p_help(void)
{
	printf(
	    "Usage: proxy\n"
	    "\n"
	    "Proxy messages between nrf radio and other communication methods.\n"
	    "\n");
}

void sig_catch_int(int signum)
{
	signal(signum, sig_catch_int);
	INFO_MSG("SIGINT/SIGTERM (CTRL-C?) caught, exit application");
	p_exit(EXIT_FAILURE);
}

void sig_catch_tstp(int signum)
{
	signal(signum, sig_catch_tstp);
	WARN_MSG("SIGTSTP (CTRL-Z?) caught, don't do that");
}

int init(int argc, char *argv[])
{
	/* very low level platform initialization */
	os_init();
	/* debug/log init */
	log_init();

	/* parse command line options */
	// if (common_options(argc, argv, opts, longopts)) {
	// 	ERROR_MSG("invalid command line option(s)\n");
	// 	p_exit(EXIT_FAILURE);
	// }

	/* signal handlers */
	signal(SIGINT, sig_catch_int);
	signal(SIGTERM, sig_catch_int);
	signal(SIGTSTP, sig_catch_tstp);

	/* spi initialization */
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

	/* nrf radio initialization */
	ERROR_IF_R(nrf24l01p_open(&nrf, &master, NRF_SS, NRF_CE), -1, "nrf24l01+ failed to initialize");
	nrf24l01p_set_channel(&nrf, KOTI_NRF_CHANNEL);
	nrf24l01p_set_speed(&nrf, KOTI_NRF_SPEED);
	/* set radio in listen mode */
	nrf24l01p_set_standby(&nrf, true);
	nrf24l01p_mode_rx(&nrf);
	nrf24l01p_set_standby(&nrf, false);

	/* broadcast initialization */
	ERROR_IF_R(broadcast_init(KOTI_BROADCAST_PORT), -1, "broadcast failed to initialize");

	return 0;
}

int main(int argc, char *argv[])
{
	/* init */
	if (init(argc, argv)) {
		ERROR_MSG("initialization failed");
		p_exit(EXIT_FAILURE);
	}

	/* program loop */
	while (1) {
		uint8_t data[32];
		if (nrf24l01p_recv(&nrf, data) > 0) {
			HEX_DUMP(data, 32, 1);
			broadcast_send(data, sizeof(data));
		} else if (broadcast_recv(data, sizeof(data)) > 0) {
			nrf24l01p_send(&nrf, data);
		} else {
			/* lets not waste all cpu if nothing happened */
			os_sleepf(0.001);
		}
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}