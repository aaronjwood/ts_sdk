/* Copyright(C) 2016 Verizon. All rights reserved. */

#include "sys.h"
#include "dbg.h"
#include "gpio_hal.h"

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
        gpio_write(pin_name, 2);
	sys_delay(500);
        gpio_write(pin_name, 1);
	}
	return 0;
}

