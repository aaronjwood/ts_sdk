/* Copyright(C) 2017 Verizon. All rights reserved. */

#include "sys.h"
#include "dbg.h"
#include "gpio_hal.h"

/* This test program is an illustration to use the gpio hal apis 
	for enabling the pins to write mode. This test uses
	hal functions for initialisation and write apis 
	to enable write over pins. */
int main(int argc, char *argv[])
{
	pin_name_t pin_name = PB7;
	gpio_config_t my_led;
	sys_init();
	my_led.dir = OUTPUT;
	my_led.pull_mode = PP_PULL_UP;
	my_led.speed = SPEED_LOW;
	gpio_init(pin_name, &my_led);

	while (1) {
	gpio_write(pin_name, PIN_LOW);
	sys_delay(500);
	gpio_write(pin_name, PIN_HIGH);
	}
	return 0;
}

