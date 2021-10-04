/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_examples.h"
#include "driver_init.h"
#include "utils.h"

static void button_on_PD00_pressed(void)
{
}

static void button_on_PB07_pressed(void)
{
}

/**
 * Example of using SteeringEncoder_IRQ
 */
void SteeringEncoder_IRQ_example(void)
{

	ext_irq_register(PIN_PD00, button_on_PD00_pressed);
	ext_irq_register(PIN_PB07, button_on_PB07_pressed);
}

/**
 * Example of using TARGET_IO to write "Hello World" using the IO abstraction.
 */
void TARGET_IO_example(void)
{
	struct io_descriptor *io;
	usart_sync_get_io_descriptor(&TARGET_IO, &io);
	usart_sync_enable(&TARGET_IO);

	io_write(io, (uint8_t *)"Hello World!", 12);
}

/**
 * Example of using ACCELERATION.
 */
void ACCELERATION_example(void)
{
	pwm_set_parameters(&ACCELERATION, 10000, 5000);
	pwm_enable(&ACCELERATION);
}

/**
 * Example of using STEERINGPOWER.
 */
void STEERINGPOWER_example(void)
{
	pwm_set_parameters(&STEERINGPOWER, 10000, 5000);
	pwm_enable(&STEERINGPOWER);
}

/**
 * Example of using BRAKE.
 */
void BRAKE_example(void)
{
	pwm_set_parameters(&BRAKE, 10000, 5000);
	pwm_enable(&BRAKE);
}
