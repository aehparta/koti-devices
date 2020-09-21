/*
 * Water usage sensor using PIC16LF18345.
 */

#include <libe/libe.h>

#pragma config FEXTOSC = OFF
#pragma config WDTE = OFF
#pragma config DEBUG = OFF
#pragma config BOREN = OFF
#pragma config LPBOREN = OFF


#define HALL_PIN_EN     GPIOC0
#define HALL_PIN_READ   GPIOB7


struct spi_master master;
struct nrf24l01p_device nrf;


int p_init(int argc, char *argv[])
{
	/* very low level platform initialization */
	os_init();
	/* debug/log init */
	// log_init();

	/* initialize spi master, hardcoded pins etc so zeroes there */
	ERROR_IF_R(spi_master_open(&master, NULL, 0, 0, 0, 0), -1, "failed to open spi master");

	/* nrf initialization, hardcoded pins so zeroes there */
	ERROR_IF_R(nrf24l01p_open(&nrf, &master, 0, 0), -1, "nrf24l01+ failed to initialize");
	/* change channel, default is 70 */
	// nrf24l01p_set_channel(&nrf, 10);
	/* change speed, default is 250k */
	// nrf24l01p_set_speed(&nrf, NRF24L01P_SPEED_2M);
	/* disable crc, default is 2 bytes */
	// nrf24l01p_set_crc(&nrf, 0);
	/* enable radio in listen mode */
	nrf24l01p_flush_rx(&nrf);
	/* nrf is fully disabled as default */
	nrf24l01p_set_standby(&nrf, true);
	nrf24l01p_set_power_down(&nrf, true);

	/* hall sensor */
	gpio_output(HALL_PIN_EN);
	gpio_low(HALL_PIN_EN);
	gpio_input(HALL_PIN_READ);

	/* Timer0 counts pulses from hall sensor */
	// T0CKIPPS = HALL_PIN_READ;
	// TMR0L = 0;
	// TMR0H = 0;
	// T0CON1 = 0x10;
	// T0CON0 = 0x90;

	/* enable interrupts globally */
	// INTCONbits.GIE = 1;
	// INTCONbits.PEIE = 1;

	/* full sleep mode */
	IDLEN = 0;

	/* disable almost all modules to save power */
	PMD0 = 0x7f;
	PMD1 = 0xfe;
	PMD2 = 0xff;
	PMD3 = 0xff;
	PMD4 = 0xfd;
	PMD5 = 0xff;

	return 0;
}

int main(int argc, char *argv[])
{
	/* init */
	p_init(argc, argv);



	gpio_high(HALL_PIN_EN);
	SLEEP();

	gpio_low(HALL_PIN_EN);
	while (1);

	/* program loop */
	INFO_MSG("starting main program loop");
	// uint8_t count = 0;
	// uint8_t x = 0;
	// while (1) {

	// 	// uint8_t xx = gpio_read(HALL_PIN_READ);
	// 	// if (x != xx) {
	// 	// 	count++;
	// 	// 	x = xx;
	// 	// 	printf("%u\r\n", count);
	// 	// }

	// 	// printf("%u\r\n", TMR0L | (TMR0H << 8));
	// 	gpio_low(HALL_PIN_EN);
	// 	os_delay_ms(3000);
	// 	gpio_high(HALL_PIN_EN);
	// 	os_delay_ms(3000);
	// }

	return EXIT_SUCCESS;
}
