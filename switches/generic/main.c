/*
 * Generic switch(es).
 */

#include <koti.h>

// struct spi_master master;

int twwwcb(struct MHD_Connection *connection, const char *url, const char *method, const char *upload_data, size_t *upload_data_size, const char **substrings, size_t substrings_c, void *userdata)
{
	char data[1024];

	sprintf(data, "got request, method: %s, url: %s\n", method, url);
	for (int i = 0; i < substrings_c; i++) {
		sprintf(data + strlen(data), "substring: %s\n", substrings[i]);
	}

	printf("%s", data);

	struct MHD_Response *response = MHD_create_response_from_buffer(strlen(data), data, MHD_RESPMEM_MUST_COPY);
	MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
	int err = MHD_queue_response(connection, 200, response);
	MHD_destroy_response(response);

	return err;
}

int p_init(int argc, char *argv[])
{
	/* very low level platform initialization */
	os_init();
	/* debug/log init */
	log_init();

#ifdef TARGET_X86
    httpd_init();
    httpd_start("0.0.0.0", 8001, "./html");
	CRIT_IF_R(httpd_register_url(NULL, "/switch/([0-9]+)/?$", twwwcb, NULL), 1, "failed to register");
#endif

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
	// 	ERROR_IF_R(nrf24l01p_koti_init(&master, NRF_SS, NRF_CE), -1, "nrf24l01+ failed to initialize");
	// 	nrf24l01p_koti_set_key((uint8_t *)"12345678", 8);

	return 0;
}

void p_exit(int retval)
{
	// nrf24l01p_koti_quit();
	// spi_master_close(&master);
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
