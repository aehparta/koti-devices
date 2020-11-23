/*
 * Water usage sensor using PIC16LF18345.
 */

#include <koti.h>

#if defined(MCU_PIC16LF18345)
#pragma config DEBUG = OFF
#endif

#pragma config FEXTOSC = OFF
#pragma config WDTE = OFF
#pragma config BOREN = OFF
#pragma config LPBOREN = OFF
#pragma config CSWEN = ON


#define HALL_PIN_EN     GPIOB7
#define HALL_PIN_READ   GPIOC7

#define HALL_SLOW_HZ    20
#define HALL_FAST_HZ    1292

#define HALL_DELAY      5
#define SEND_DELAY      60

#define SLOW_TIMER      193 /* 20 Hz */
#define FAST_TIMER      3 /* 1292 Hz */

/* nrf irq pin, not used but must be input */
#define NRF_PIN_IRQ     GPIOC6


struct spi_master master;
#ifdef USE_BLE
struct nrf24l01p_ble_device nrf_ble;
uint8_t mac[6] = { 0x17, 0x17, 'B', 'E', 'B', 'T' };
#endif


void p_init(void)
{
	/* switch to 32MHz clock */
	OSCCON1 = 0x60;
	OSCCON3 = 0x00;
	OSCEN = 0x00;
	OSCFRQ = 0x06;
	OSCTUNE = 0x00;

	/* very low level platform initialization */
	os_init();
	/* debug/log init */
	// log_init();

	/* all unused gpios as output and low to save power */
	gpio_output(GPIOA0);
	gpio_low(GPIOA0);
	gpio_output(GPIOA1);
	gpio_low(GPIOA1);
	gpio_output(GPIOA2);
	gpio_low(GPIOA2);
	gpio_output(GPIOA4);
	gpio_low(GPIOA4);
	gpio_output(GPIOA5);
	gpio_low(GPIOA5);

	gpio_output(GPIOC0);
	gpio_low(GPIOC0);
	gpio_output(GPIOC3);
	gpio_low(GPIOC3);
	gpio_output(GPIOC4);
	gpio_low(GPIOC4);
	gpio_output(GPIOC5);
	gpio_low(GPIOC5);

	/* disable all modules and enable only needed parts to save power */
	PMD0 = 0xff;
	PMD1 = 0xff;
	PMD2 = 0xff;
	PMD3 = 0xff;
	PMD4 = 0xff;
	PMD5 = 0xff;
#if defined(MCU_PIC16LF18446)
	PMD6 = 0xff;
	PMD7 = 0xff;
#endif

	/* enable some of the needed modules */
	SYSCMD = 0;
	MSSP1MD = 0;
	TMR0MD = 0;

	/* hall sensor */
	gpio_input(HALL_PIN_READ);
	gpio_output(HALL_PIN_EN);
	gpio_high(HALL_PIN_EN);

	/* initialize spi master, hardcoded pins etc so zeroes there */
	spi_master_open(&master, NULL, 0, 0, 0, 0);

	/* nrf initialization, hardcoded pins so zeroes there */
#ifndef USE_BLE
	nrf24l01p_koti_init(&master, 0, 0);
	nrf24l01p_koti_set_key((uint8_t *)"12345678", 8);
#else
	/* nrf ble initialization */
	nrf24l01p_ble_open(&nrf_ble, &master, 0, 0, mac);
	nrf24l01p_set_power_down(&nrf_ble.nrf, true);
#endif

	/* disable system clock, we don't need it until later */
	SYSCMD = 1;

	/* nrf irq as input, not used though */
	// gpio_input(NRF_PIN_IRQ);

	/* timer 0 setup */
	TMR0L = 0;
	TMR0H = SLOW_TIMER; /* with 1:8 prescaler this makes 20Hz (50 ms intervals) */
	T0CON1 = 0x93; /* LFINTOSC and async, 1:8 prescaler */
	T0CON0 = 0x80; /* enable timer as 8-bit, no postscaler */
	TMR0IE = 1;

	/* full sleep mode */
	IDLEN = 0;
}

void main(void)
{
	/* init */
	p_init();

	uint8_t hall_last = gpio_read(HALL_PIN_READ);
	uint16_t hall_changed = 0;
	uint64_t hall_ticks = 0;
	uint16_t send_timer = HALL_DELAY * HALL_SLOW_HZ;

#ifndef USE_BLE
	struct koti_nrf_pck_broadcast_uuid pck;
	memcpy(pck.uuid, "123456789abcdef", 16);
#else
	uint8_t buf[18];
	uint8_t l = 0;
#endif


	// ADCMD = 0;
	// FVRMD = 0;
	// while (!FVRRDY);
	// FVRCON = 0x81;
	// ADCLK = 0;
	// ADCON2 = 0x00;
	// ADREF = 0x00;
	// ADPCH = 0x3e;
	// ADCON0bits.ON = 1;
	// while (1) {
	// 	ADCON0bits.GO = 1;
	// 	while (ADCON0bits.GO);
	// 	pck.hdr.bat = (uint8_t)((2048.0 * 16.0 / (float)ADRES) * 2048.0 / 100.0);
	// 	pck.hdr.flags = 0;
	// 	pck.hdr.type = 0;
	// 	nrf24l01p_koti_send(KOTI_NRF_ID_BRIDGE, KOTI_NRF_ID_UUID, &pck);
	// 	os_delay_ms(300);
	// }




	// while (1) {
	// 	memset(pck.data, 0, 8);

	// 	SYSCMD = 0;

	// 	/* enable adc/fvr */
	// 	FVRMD = 0;
	// 	ADCMD = 0;
	// 	while (!FVRRDY);
	// 	FVRCON = 0x81;
	// 	ADPCH = 0x3e;
	// 	ADCON0 = 0x80;

	// 	/* do one conversion before actual conversion, for some reason things are not stable yet */
	// 	ADCON0bits.GO = 1;
	// 	while (ADCON0bits.GO);

	// 	/* calculate battery voltage with 100mV precision */
	// 	ADCON0bits.GO = 1;
	// 	while (ADCON0bits.GO);
	// 	pck.hdr.bat = (uint8_t)((2048.0 * 16.0 / (float)ADRES) * 2048.0 / 100.0);
	// 	pck.u16[0] = ADRES >> 4;

	// 	/* disable adc/fvr */
	// 	ADCMD = 1;
	// 	FVRMD = 1;

	// 	/* send */
	// 	pck.hdr.flags = 0;
	// 	pck.hdr.type = 0;
	// 	nrf24l01p_koti_send(KOTI_NRF_ID_BRIDGE, KOTI_NRF_ID_UUID, &pck);


	// 	SYSCMD = 1;

	// 	os_delay_ms(300);
	// }

	// while (1) {
	// 	SLEEP();
	// 	TMR0IF = 0;
	// }

	while (1) {
		SLEEP();
		TMR0IF = 0;

		/* read hall */
		uint8_t hall_now = gpio_read(HALL_PIN_READ);

		/* trigger new measurement */
		gpio_low(HALL_PIN_EN);
		os_delay_us(1);
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
				/* start dozing */
			}
		} else {
			if (!send_timer) {
				/* enable system clock */
				SYSCMD = 0;

				/* enable adc/fvr */
				FVRMD = 0;
				ADCMD = 0;
				while (!FVRRDY);
				FVRCON = 0x81;
				ADPCH = 0x3e;
				ADCLK = 2;
				ADCON0 = 0x80;

				/* do one conversion before actual conversion, for some reason things are not stable yet */
				ADCON0bits.GO = 1;
				while (ADCON0bits.GO);

				/* calculate battery voltage with 100mV precision */
				ADCON0bits.GO = 1;
				while (ADCON0bits.GO);
				uint8_t vbat = (uint8_t)((2048.0 * 16.0 / (float)ADRES) * 2048.0 / 100.0);

				/* disable adc/fvr */
				ADCMD = 1;
				FVRMD = 1;

				/* send */
#ifndef USE_BLE
				pck.hdr.flags = 0;
				pck.hdr.type = 0;
				pck.hdr.bat = vbat | vbat < 19 ? KOTI_NRF_BAT_EMPTY_MASK : 0; /* do rough estimation of battery being empty: under 1.9 volts is quite empty */
				pck.u64 = hall_ticks;
				nrf24l01p_koti_send(KOTI_NRF_ID_BRIDGE, KOTI_NRF_ID_UUID, &pck);
#else
				/* calculate rough battery level using voltage as linear indicator (not exactly accurate..):
				 *  - 2.8 volts or above is full
				 *  - 1.9 volts or under is empty
				 */
				vbat = vbat < 19 ? 0 : vbat - 18;
				vbat *= 10;
				vbat = vbat > 100 ? 100 : vbat;

				buf[0] = 4;
				buf[1] = 0x16;
				buf[2] = 0x0f;
				buf[3] = 0x18;
				buf[4] = vbat;

				uint64_t litres = hall_ticks / 1200;

				buf[5] = 7;
				buf[6] = 0x16;
				buf[7] = 0x67;
				buf[8] = 0x27;
				buf[9] = litres & 0xff;
				buf[10] = (litres >> 8) & 0xff;
				buf[11] = (litres >> 16) & 0xff;
				buf[12] = (litres >> 24) & 0xff;

				for (uint8_t i = 0; i < 3; i++) {
					nrf24l01p_ble_advertise(&nrf_ble, buf, 13);
					nrf24l01p_ble_hop(&nrf_ble);
				}
#endif
				send_timer = SEND_DELAY * HALL_SLOW_HZ;

				/* disable system clock */
				SYSCMD = 1;
			}
			send_timer--;
		}
	}
}
