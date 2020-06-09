/*
 * Basic PWM driver for esp32
 */

#include <libe/libe.h>
#include <driver/mcpwm.h>
#include "pwm.h"


int pwm_init(void)
{
	mcpwm_config_t cfg;

	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, 2);
	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, 4);
	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, 18);
	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1B, 23);
	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM2A, 19);
	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM2B, 22);

	cfg.frequency = 20000;
	cfg.cmpr_a = 0;
	cfg.cmpr_b = 0;
	cfg.counter_mode = MCPWM_UP_COUNTER;
	cfg.duty_mode = MCPWM_DUTY_MODE_0;
	mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &cfg);

	cfg.frequency = 20000;
	cfg.cmpr_a = 0;
	cfg.cmpr_b = 0;
	cfg.counter_mode = MCPWM_UP_COUNTER;
	cfg.duty_mode = MCPWM_DUTY_MODE_0;
	mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &cfg);

	cfg.frequency = 20000;
	cfg.cmpr_a = 0;
	cfg.cmpr_b = 0;
	cfg.counter_mode = MCPWM_UP_COUNTER;
	cfg.duty_mode = MCPWM_DUTY_MODE_0;
	mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_2, &cfg);

	INFO_MSG("PWM timer 0 frequency is: %d", mcpwm_get_frequency(MCPWM_UNIT_0, MCPWM_TIMER_0));
	INFO_MSG("PWM timer 1 frequency is: %d", mcpwm_get_frequency(MCPWM_UNIT_0, MCPWM_TIMER_1));
	INFO_MSG("PWM timer 2 frequency is: %d", mcpwm_get_frequency(MCPWM_UNIT_0, MCPWM_TIMER_2));

	return 0;
}

void pwm_set_duty(int channel, float duty)
{
	switch (channel) {
	case 0:
		mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty);
		break;
	case 1:
		mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, duty);
		break;
	case 2:
		mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, duty);
		break;
	case 3:
		mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_B, duty);
		break;
	case 4:
		mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_2, MCPWM_OPR_A, duty);
		break;
	case 5:
		mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_2, MCPWM_OPR_B, duty);
		break;
	}
}

