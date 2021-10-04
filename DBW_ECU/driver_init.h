/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */
#ifndef DRIVER_INIT_INCLUDED
#define DRIVER_INIT_INCLUDED

#include "atmel_start_pins.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_atomic.h>
#include <hal_delay.h>
#include <hal_gpio.h>
#include <hal_init.h>
#include <hal_io.h>
#include <hal_sleep.h>

#include <hal_ext_irq.h>

#include <hal_usart_sync.h>
#include <hal_pwm.h>
#include <hpl_tc_base.h>
#include <hal_pwm.h>
#include <hpl_tc_base.h>
#include <hal_pwm.h>
#include <hpl_tc_base.h>

#include <hal_mac_async.h>

extern struct usart_sync_descriptor TARGET_IO;

extern struct pwm_descriptor ACCELERATION;

extern struct pwm_descriptor STEERINGPOWER;

extern struct pwm_descriptor BRAKE;

extern struct mac_async_descriptor COMMUNICATION_IO;

void TARGET_IO_PORT_init(void);
void TARGET_IO_CLOCK_init(void);
void TARGET_IO_init(void);

void ACCELERATION_PORT_init(void);
void ACCELERATION_CLOCK_init(void);
void ACCELERATION_init(void);

void STEERINGPOWER_PORT_init(void);
void STEERINGPOWER_CLOCK_init(void);
void STEERINGPOWER_init(void);

void BRAKE_PORT_init(void);
void BRAKE_CLOCK_init(void);
void BRAKE_init(void);

void COMMUNICATION_IO_CLOCK_init(void);
void COMMUNICATION_IO_init(void);
void COMMUNICATION_IO_PORT_init(void);
void COMMUNICATION_IO_example(void);

/**
 * \brief Perform system initialization, initialize pins and clocks for
 * peripherals
 */
void system_init(void);

#ifdef __cplusplus
}
#endif
#endif // DRIVER_INIT_INCLUDED
