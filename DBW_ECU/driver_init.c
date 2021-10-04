/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_init.h"
#include <peripheral_clk_config.h>
#include <utils.h>
#include <hal_init.h>

struct usart_sync_descriptor TARGET_IO;

struct pwm_descriptor ACCELERATION;

struct pwm_descriptor STEERINGPOWER;

struct pwm_descriptor BRAKE;

struct mac_async_descriptor COMMUNICATION_IO;

void SteeringEncoder_IRQ_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, EIC_GCLK_ID, CONF_GCLK_EIC_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_mclk_set_APBAMASK_EIC_bit(MCLK);

	// Set pin direction to input
	gpio_set_pin_direction(encoder_A, GPIO_DIRECTION_IN);

	gpio_set_pin_pull_mode(encoder_A,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_UP);

	gpio_set_pin_function(encoder_A, PINMUX_PD00A_EIC_EXTINT0);

	// Set pin direction to input
	gpio_set_pin_direction(encoder_B, GPIO_DIRECTION_IN);

	gpio_set_pin_pull_mode(encoder_B,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_UP);

	gpio_set_pin_function(encoder_B, PINMUX_PB07A_EIC_EXTINT7);

	ext_irq_init();
}

void TARGET_IO_PORT_init(void)
{

	gpio_set_pin_function(PB25, PINMUX_PB25D_SERCOM2_PAD0);

	gpio_set_pin_function(PB24, PINMUX_PB24D_SERCOM2_PAD1);
}

void TARGET_IO_CLOCK_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM2_GCLK_ID_CORE, CONF_GCLK_SERCOM2_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM2_GCLK_ID_SLOW, CONF_GCLK_SERCOM2_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	hri_mclk_set_APBBMASK_SERCOM2_bit(MCLK);
}

void TARGET_IO_init(void)
{
	TARGET_IO_CLOCK_init();
	usart_sync_init(&TARGET_IO, SERCOM2, (void *)NULL);
	TARGET_IO_PORT_init();
}

void ACCELERATION_PORT_init(void)
{

	gpio_set_pin_function(acceleration, PINMUX_PA05E_TC0_WO1);
}

void ACCELERATION_CLOCK_init(void)
{

	hri_mclk_set_APBAMASK_TC0_bit(MCLK);
	hri_gclk_write_PCHCTRL_reg(GCLK, TC0_GCLK_ID, CONF_GCLK_TC0_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
}

void ACCELERATION_init(void)
{
	ACCELERATION_CLOCK_init();
	ACCELERATION_PORT_init();
	pwm_init(&ACCELERATION, TC0, _tc_get_pwm());
}

void STEERINGPOWER_PORT_init(void)
{

	gpio_set_pin_function(steering_power, PINMUX_PB09E_TC4_WO1);
}

void STEERINGPOWER_CLOCK_init(void)
{

	hri_mclk_set_APBCMASK_TC4_bit(MCLK);
	hri_gclk_write_PCHCTRL_reg(GCLK, TC4_GCLK_ID, CONF_GCLK_TC4_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
}

void STEERINGPOWER_init(void)
{
	STEERINGPOWER_CLOCK_init();
	STEERINGPOWER_PORT_init();
	pwm_init(&STEERINGPOWER, TC4, _tc_get_pwm());
}

void BRAKE_PORT_init(void)
{

	gpio_set_pin_function(PB15, PINMUX_PB15E_TC5_WO1);
}

void BRAKE_CLOCK_init(void)
{

	hri_mclk_set_APBCMASK_TC5_bit(MCLK);
	hri_gclk_write_PCHCTRL_reg(GCLK, TC5_GCLK_ID, CONF_GCLK_TC5_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
}

void BRAKE_init(void)
{
	BRAKE_CLOCK_init();
	BRAKE_PORT_init();
	pwm_init(&BRAKE, TC5, _tc_get_pwm());
}

void COMMUNICATION_IO_PORT_init(void)
{

	gpio_set_pin_function(PC11, PINMUX_PC11L_GMAC_GMDC);

	gpio_set_pin_function(PC12, PINMUX_PC12L_GMAC_GMDIO);

	gpio_set_pin_function(PA13, PINMUX_PA13L_GMAC_GRX0);

	gpio_set_pin_function(PA12, PINMUX_PA12L_GMAC_GRX1);

	gpio_set_pin_function(PC20, PINMUX_PC20L_GMAC_GRXDV);

	gpio_set_pin_function(PA15, PINMUX_PA15L_GMAC_GRXER);

	gpio_set_pin_function(PA18, PINMUX_PA18L_GMAC_GTX0);

	gpio_set_pin_function(PA19, PINMUX_PA19L_GMAC_GTX1);

	gpio_set_pin_function(PA14, PINMUX_PA14L_GMAC_GTXCK);

	gpio_set_pin_function(PA17, PINMUX_PA17L_GMAC_GTXEN);
}

void COMMUNICATION_IO_CLOCK_init(void)
{
	hri_mclk_set_AHBMASK_GMAC_bit(MCLK);
	hri_mclk_set_APBCMASK_GMAC_bit(MCLK);
}

void COMMUNICATION_IO_init(void)
{
	COMMUNICATION_IO_CLOCK_init();
	mac_async_init(&COMMUNICATION_IO, GMAC);
	COMMUNICATION_IO_PORT_init();
}

void COMMUNICATION_IO_example(void)
{
	mac_async_enable(&COMMUNICATION_IO);
	mac_async_write(&COMMUNICATION_IO, (uint8_t *)"Hello World!", 12);
}

void system_init(void)
{
	init_mcu();

	// GPIO on PA06

	gpio_set_pin_level(SteeringEnable,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(SteeringEnable, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(SteeringEnable, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PA07

	gpio_set_pin_level(SteeringDirection,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(SteeringDirection, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(SteeringDirection, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PB01

	gpio_set_pin_level(SafetyLights2Enable,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(SafetyLights2Enable, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(SafetyLights2Enable, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PB06

	gpio_set_pin_level(SafetyLights1Enable,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(SafetyLights1Enable, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(SafetyLights1Enable, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PC01

	gpio_set_pin_level(AccelerationEnable,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(AccelerationEnable, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(AccelerationEnable, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PC03

	// Set pin direction to input
	gpio_set_pin_direction(EStop_In, GPIO_DIRECTION_IN);

	gpio_set_pin_pull_mode(EStop_In,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_DOWN);

	gpio_set_pin_function(EStop_In, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PC10

	gpio_set_pin_level(NotReverse,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(NotReverse, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(NotReverse, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PC30

	gpio_set_pin_level(Reverse,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(Reverse, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(Reverse, GPIO_PIN_FUNCTION_OFF);

	SteeringEncoder_IRQ_init();

	TARGET_IO_init();

	ACCELERATION_init();

	STEERINGPOWER_init();

	BRAKE_init();

	COMMUNICATION_IO_init();
}
