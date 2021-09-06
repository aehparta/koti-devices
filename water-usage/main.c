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

#define HALL_PIN_EN GPIOB7
#define HALL_PIN_EN_BIT LATBbits.LATB7
#define HALL_PIN_READ GPIOC7

#define TIMER_HZ 20
#define HALL_PULSES_CNT (65536 - 60) /* pulses per litre */
#define HALL_WAIT 2                  /* seconds */

#define SEND_DELAY 10

struct spi_master master;
#ifdef USE_BLE
struct nrf24l01p_ble_device nrf_ble;
uint8_t mac[6] = {0x17, 0x17, 'B', 'E', 'B', 'T'};
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
	TMR1MD = 0;

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

	/* timer 0 setup */
	TMR0L = 0;
	TMR0H = 31000 / TIMER_HZ / 16;
	T0CON1 = 0x94; /* LFINTOSC and async, 1:16 prescaler */
	TMR0IE = 1;
	T0CON0 = 0x80; /* enable timer as 8-bit, no postscaler */

	/* timer 1 setup */
	TMR1H = HALL_PULSES_CNT >> 8;
	TMR1L = HALL_PULSES_CNT & 0xff;
	T1GCON = 0x00;
	TMR1GATE = 0;
	TMR1CLK = 0;
	T1CKIPPS = HALL_PIN_READ;
	TMR1IE = 1;
	T1CON = 0x07;

	/* full sleep mode */
	IDLEN = 0;
}

void main(void)
{
	/* init */
	p_init();

#ifndef USE_BLE
	struct koti_nrf_pck_broadcast_uuid pck;
	memset(pck.data, 0, sizeof(pck.data));
	memcpy(pck.uuid, "123456789abcdef", 16);
#else
	uint8_t buf[18];
#endif

	uint32_t litres = 0;
	uint16_t send_timer = SEND_DELAY * TIMER_HZ;
	uint8_t hall_delay = 0;
	uint8_t hall_last = HALL_PULSES_CNT & 0xff;
	int vbat = 0;

	while (1) {
		SYSCMD = 1;
		SLEEP();
		HALL_PIN_EN_BIT = 0;
		SYSCMD = 0;

		if (hall_last != TMR1L) {
			hall_last = TMR1L;
			hall_delay = HALL_WAIT * TIMER_HZ;
		} else if (hall_delay == 0) {
			HALL_PIN_EN_BIT = 1;
		}

		if (TMR1IF) {
			TMR1IF = 0;
			TMR1H = HALL_PULSES_CNT >> 8;
			TMR1L = HALL_PULSES_CNT & 0xff;
			litres++;
		}

		if (TMR0IF) {
			TMR0IF = 0;

			if (hall_delay) {
				hall_delay--;
			}

			send_timer--;
			if (send_timer == 0) {
				send_timer = SEND_DELAY * TIMER_HZ;

				/* enable adc/fvr */
				FVRMD = 0;
				ADCMD = 0;
				while (!FVRRDY)
					;
				FVRCON = 0x81;
				ADPCH = 0x3e;
				ADCLK = 2;
				ADCON0 = 0x80;

				/* do one conversion before actual conversion, for some reason things are not stable yet */
				ADCON0bits.GO = 1;
				while (ADCON0bits.GO)
					;

				/* calculate battery voltage with 100mV precision */
				ADCON0bits.GO = 1;
				while (ADCON0bits.GO)
					;

				/* (2048 * 2048 * 16) / ADRES = millivolts:
				 *  - ADRES = 22370 = 3.0V or ADRESH = 87
				 *  - ADRES = 23140 = 2.9V
				 *  - ADRES = 23967 = 2.8V
				 *  - ADRES = 33554 = 2.0V
				 *  - ADRES = 35320 = 1.9V or ADRESH = 137
				 */
				if (ADRESH > 137) {
					vbat = 0;
				} else {
					vbat = -(ADRESH);
					vbat += 137;
					vbat = vbat < 50 ? vbat * 2 : 100;
				}

				/* disable adc/fvr */
				ADCMD = 1;
				FVRMD = 1;

				/* send data */
#ifndef USE_BLE
				pck.hdr.flags = 0;
				pck.hdr.type = 0;
				pck.hdr.bat = (uint8_t)vbat;
				pck.u32 = litres;
				nrf24l01p_koti_send(KOTI_NRF_ID_BRIDGE, KOTI_NRF_ID_UUID, &pck);
#else
				buf[0] = 4;
				buf[1] = 0x16;
				buf[2] = 0x0f;
				buf[3] = 0x18;
				buf[4] = (uint8_t)vbat;

				buf[5] = 7;
				buf[6] = 0x16;
				buf[7] = 0x67;
				buf[8] = 0x27;
				buf[9] = (litres >> 24) & 0xff;
				buf[10] = (litres >> 16) & 0xff;
				buf[11] = (litres >> 8) & 0xff;
				buf[12] = litres & 0xff;

				for (uint8_t i = 0; i < 3; i++) {
					nrf24l01p_ble_advertise(&nrf_ble, buf, 13);
					nrf24l01p_ble_hop(&nrf_ble);
				}
#endif
			}
		}
	}
}
