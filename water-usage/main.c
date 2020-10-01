/*
 * Water usage sensor using PIC16LF18345.
 */

#include <koti.h>

#pragma config FEXTOSC = OFF
#pragma config WDTE = OFF
#pragma config DEBUG = OFF
#pragma config BOREN = OFF
#pragma config LPBOREN = OFF


#define HALL_PIN_EN     GPIOC0
#define HALL_PIN_READ   GPIOB7

#define HALL_SLOW_HZ    20
#define HALL_FAST_HZ    1292

#define HALL_DELAY      5
#define SEND_DELAY      60

#define SLOW_TIMER      193 /* 20 Hz */
#define FAST_TIMER      3 /* 1292 Hz */


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

	/* hall sensor */
	gpio_output(HALL_PIN_EN);
	gpio_high(HALL_PIN_EN);
	gpio_input(HALL_PIN_READ);

	/* all other gpios as output and high */
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
	gpio_output(GPIOC3);
	gpio_high(GPIOC3);
	gpio_output(GPIOC4);
	gpio_high(GPIOC4);
	gpio_output(GPIOC5);
	gpio_high(GPIOC5);
	gpio_output(GPIOC6);
	gpio_high(GPIOC6);
	gpio_output(GPIOC7);
	gpio_high(GPIOC7);

	/* full sleep mode */
	IDLEN = 0;

	/* disable almost all modules to save power */
	PMD0 = 0x7f; /* system clock enabled */
	PMD1 = 0xfe; /* timer 0 enabled */
	PMD2 = 0xff;
	PMD3 = 0xff;
	PMD4 = 0xfd; /* spi enabled */
	PMD5 = 0xff;

	/* Timer0 wakes up the cpu once and a while */
	TMR0L = 0;
	TMR0H = SLOW_TIMER; /* with 1:8 prescaler this makes 20Hz (50 ms intervals) */
	T0CON1 = 0x93; /* LFINTOSC and async, 1:8 prescaler */
	T0CON0 = 0x80; /* enable timer as 8-bit, no postscaler */
	PIE0bits.TMR0IE = 1;

	/* enable interrupts globally */
	// INTCONbits.GIE = 1;
	// INTCONbits.PEIE = 1;

}

void main(void)
{
	/* init */
	p_init();

	uint8_t hall_last = gpio_read(HALL_PIN_READ);
	uint16_t hall_changed = 0;
	uint64_t hall_ticks = 0;
	uint16_t send_timer = HALL_DELAY * HALL_SLOW_HZ;
	struct koti_nrf_pck_broadcast_uuid pck;
	memcpy(pck.uuid, "123456789abcdef", 16);

	while (1) {
		SLEEP();
		PIR0bits.TMR0IF = 0;

		/* read hall */
		uint8_t hall_now = gpio_read(HALL_PIN_READ);

		/* trigger new measurement */
		gpio_low(HALL_PIN_EN);
		gpio_high(HALL_PIN_EN);

		/* check for changes */
		if (hall_now != hall_last) {
			hall_last = hall_now;
			hall_ticks++;

			if (!hall_changed) {
				TMR0H = FAST_TIMER;
			}
			/* wait a while for things to cool down */
			hall_changed = HALL_DELAY * HALL_FAST_HZ;
		} else if (hall_changed) {
			hall_changed--;
			if (!hall_changed) {
				/* back to slow playing */
				TMR0H = SLOW_TIMER;
				/* send shortly after no changes in hall */
				send_timer = HALL_DELAY * HALL_SLOW_HZ;
			}
		} else {
			if (!send_timer) {
				/* send */
				pck.hdr.flags = 0;
				pck.hdr.type = 0;
				pck.hdr.res1 = 0;
				pck.u64 = hall_ticks;
				nrf24l01p_koti_send(KOTI_NRF_ID_BRIDGE, KOTI_NRF_ID_UUID, &pck);
				send_timer = SEND_DELAY * HALL_SLOW_HZ;
			}
			send_timer--;
		}
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
