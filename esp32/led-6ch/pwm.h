/*
 * Basic PWM driver for esp32
 */

#ifndef _PWM_H_
#define _PWM_H_

int pwm2_init(void);
void pwm2_set_duty(int channel, float duty);

#endif /* _PWM_H_ */
