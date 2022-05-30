/*
 * Water usage sensor using PIC16LF18345.
 */

#include <koti.h>
#include "device.h"

#ifdef TARGET_PIC8
#if defined(MCU_PIC16LF18345)
#pragma config DEBUG = OFF
#endif
#pragma config FEXTOSC = OFF
#pragma config WDTE = OFF
#pragma config BOREN = OFF
#pragma config LPBOREN = OFF
#pragma config CSWEN = ON

#define CLICK_PIN_EN GPIOB7
#define CLICK_PIN_EN_BIT LATBbits.LATB7
#define CLICK_PIN_READ GPIOC7
#endif

#define SEND_DELAY_BATTERY 600

struct spi_master master;
static const uint8_t uuid[] = UUID_ARRAY;
struct koti_nrf_pck pck;


void p_init(void)
{
	/* very low level platform initialization */
	os_init();
	/* debug/log init */
	log_init();

#ifdef TARGET_PIC8
	/* switch to 32MHz clock */
	OSCCON1 = 0x60;
	OSCCON3 = 0x00;
	OSCEN = 0x00;
	OSCFRQ = 0x06;
	OSCTUNE = 0x00;

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

	/* timer 0 setup */
	TMR0L = 0;
	TMR0H = 31000 / 16;
	T0CON1 = 0x94; /* LFINTOSC and async, 1:16 prescaler */
	TMR0IE = 1;
	T0CON0 = 0x80; /* enable timer as 8-bit, no postscaler */

	/* timer 1 setup */
	TMR1H = 0;
	TMR1L = 0;
	T1GCON = 0x00;
	TMR1GATE = 0;
	TMR1CLK = 0;
	T1CKIPPS = COUNT_PIN_READ;
	TMR1IE = 1;
	T1CON = 0x07;

	/* full sleep mode */
	IDLEN = 0;

	/* initialize spi master, hardcoded pins etc so zeroes there */
	spi_master_open(&master, NULL, 0, 0, 0, 0);
#endif

	/* nrf initialization, hardcoded pins so zeroes there */
	nrf24l01p_koti_init(&master, 0, 0);
	nrf24l01p_koti_set_key((uint8_t *)KEY_STRING, sizeof(KEY_STRING) - 1);
}

void send_click(void)
{
	memset(&pck, 0, sizeof(pck) - sizeof(uuid));
	pck.hdr.src = KOTI_NRF_ADDR_BROADCAST;
	pck.hdr.dst = KOTI_NRF_ADDR_CTRL;
	pck.hdr.flags = KOTI_NRF_FLAG_ENC_RC5_2_BLOCKS;
	pck.hdr.type = KOTI_TYPE_CLICK;
	memcpy(pck.uuid, uuid, sizeof(uuid));
	nrf24l01p_koti_send(&pck);
}

void send_battery(uint8_t percentage, uint8_t type, uint8_t cells, float voltage)
{
	memset(&pck, 0, sizeof(pck) - sizeof(uuid));
	pck.hdr.src = KOTI_NRF_ADDR_BROADCAST;
	pck.hdr.dst = KOTI_NRF_ADDR_CTRL;
	pck.hdr.flags = KOTI_NRF_FLAG_ENC_RC5_2_BLOCKS;
	pck.hdr.type = KOTI_TYPE_PSU;
	pck.psu.percentage = percentage;
	pck.psu.type = type;
	pck.psu.cells = cells;
	pck.psu.voltage = voltage;
	memcpy(pck.uuid, uuid, sizeof(uuid));
	nrf24l01p_koti_send(&pck);
}

int main(void)
{
	/* init */
	p_init();

#ifdef TARGET_PIC8
	uint16_t send_timer_battery = SEND_DELAY_BATTERY;

	while (1) {
		SYSCMD = 1;
		SLEEP();
		SYSCMD = 0;

		if (TMR0IF) {
			TMR0IF = 0;

			send_timer_battery--;

			if (send_timer_battery == 0) {
				uint8_t percentage = 0;

				send_timer_battery = SEND_DELAY_BATTERY;

				/* enable adc/fvr */
				FVRMD = 0;
				ADCMD = 0;
				while (!FVRRDY) {}
				FVRCON = 0x81;
				ADPCH = 0x3e;
				ADCLK = 2;
				ADCON0 = 0x80;

				/* do one conversion before actual conversion, for some reason things are not stable yet */
				ADCON0bits.GO = 1;
				while (ADCON0bits.GO) {}

				/* calculate battery voltage with 100mV precision */
				ADCON0bits.GO = 1;
				while (ADCON0bits.GO) {}

				/* (2048 * 2048 * 16) / ADRES = millivolts:
				 *  - ADRES = 22370 = 3.0V or ADRESH = 87
				 *  - ADRES = 23140 = 2.9V
				 *  - ADRES = 23967 = 2.8V
				 *  - ADRES = 33554 = 2.0V
				 *  - ADRES = 35320 = 1.9V or ADRESH = 137
				 */
				if (ADRESH <= 137) {
					percentage = 137 - (ADRESH);
					percentage = percentage < 50 ? percentage * 2 : 100;
				}

				/* disable adc/fvr */
				ADCMD = 1;
				FVRMD = 1;

				send_battery(percentage, KOTI_PSU_BATTERY_LITHIUM, 0, 0);
			}
		}
	}
#else
	while (1) {
		send_click();
		send_battery(75, KOTI_PSU_BATTERY_LITHIUM, 1, 2.5);
		os_delay_ms(1000);
	}
#endif

	return 0;
}
