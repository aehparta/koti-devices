/*
 * Water usage sensor using PIC16LF18345.
 */

#include <koti.h>

#pragma config FEXTOSC = OFF
#pragma config WDTE = OFF
#pragma config DEBUG = OFF
#pragma config BOREN = OFF
#pragma config LPBOREN = OFF


#define BTN1            GPIOC4
#define BTN2            GPIOC5


struct spi_master master;


void p_init(void)
{
	/* very low level platform initialization */
	os_init();
	/* debug/log init */
	// log_init();

	/* initialize spi master, hardcoded pins etc so zeroes there */
	spi_master_open(&master, NULL, 0, 0, 0, 0);

	/* nrf initialization, hardcoded pins so zeroes there */
	nrf24l01p_koti_init(&master, 0, 0);
	nrf24l01p_koti_set_key((uint8_t *)"12345678", 8);

	/* buttons as inputs and enable interrupts on both edges */
	gpio_input(BTN1);
	gpio_input(BTN2);
	IOCCP = 0x30;
	IOCCN = 0x30;

	/* all other gpios as output and high to save power */
	gpio_output(GPIOA0);
	gpio_high(GPIOA0);
	gpio_output(GPIOA1);
	gpio_high(GPIOA1);
	gpio_output(GPIOA2);
	gpio_high(GPIOA2);
	gpio_output(GPIOA4);
	gpio_high(GPIOA4);
	gpio_output(GPIOA5);
	gpio_high(GPIOA5);

	gpio_output(GPIOB7);
	gpio_high(GPIOB7);

	gpio_output(GPIOC0);
	gpio_high(GPIOC0);
	gpio_output(GPIOC3);
	gpio_high(GPIOC3);
	gpio_output(GPIOC6);
	gpio_high(GPIOC6);
	gpio_output(GPIOC7);
	gpio_high(GPIOC7);

	/* full sleep mode */
	IDLEN = 0;

	/* disable almost all modules to save power */
	PMD0 = 0x7e; /* system clock and pin interrupts enabled */
	PMD1 = 0xff;
	PMD2 = 0xff;
	PMD3 = 0xff;
	PMD4 = 0xfd; /* spi enabled */
	PMD5 = 0xff;

	/* enable pint interrupts */
	PIE0bits.IOCIE = 1;

	/* Timer0 wakes up the cpu once and a while */
	// TMR0L = 0;
	// TMR0H = 250;
	// T0CON1 = 0x99; /* LFINTOSC and async, 1:8 prescaler */
	// T0CON0 = 0x80; /* enable timer as 8-bit, no postscaler */
	// PIE0bits.TMR0IE = 1;

	/* enable interrupts globally */
	// INTCONbits.GIE = 1;
	// INTCONbits.PEIE = 1;

}

void main(void)
{
	/* init */
	p_init();

	struct koti_nrf_pck_broadcast_uuid pck;
	memcpy(pck.uuid, "koti-switch-test", 16);

	while (1) {
		SLEEP();
		IOCCF = 0;

		/* read hall */
		uint8_t btn1 = gpio_read(BTN1);
		uint8_t btn2 = gpio_read(BTN2);

		/* send */
		pck.hdr.flags = 0;
		pck.hdr.bat = 0;
		pck.hdr.type = KOTI_NRF_TYPE_BUTTONS;
		memset(pck.data, 0, sizeof(pck.data));
		pck.data[0] = 2;
		pck.data[1] = (btn1 ? 0 : 1) | (btn2 ? 0 : 2);
		nrf24l01p_koti_send(KOTI_NRF_ID_BRIDGE, KOTI_NRF_ID_UUID, &pck);
	}

	/* program loop */
	// while (1) {
	// 	struct koti_nrf_pck_broadcast_uuid pck;

	// 	os_wdt_reset();

	// 	memset(&pck, 0, sizeof(pck));
	// 	memcpy(pck.uuid, "123456789abcdef", 16);
	// 	pck.u64 = x;
	// 	x++;

	// 	nrf24l01p_koti_send(KOTI_NRF_ID_BRIDGE, KOTI_NRF_ID_UUID, &pck);

	// 	os_delay_ms(1000);
	// }
}
