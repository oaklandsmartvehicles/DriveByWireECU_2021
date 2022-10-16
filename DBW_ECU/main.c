/**
 * \file
 *
 * \brief LwIP socket api application implementation
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

#include "atmel_start.h"
#include "lwip_socket_api.h"
#include "hal_gpio.h"
#include "hal_delay.h"
#include "hal_usart_sync.h"
#include "DriveByWireIO.h"
#include "driver_examples.h"
#include "EthernetIO.h"
#include "FreeRTOS.h"
#include "main_context.h"
#include "PID.h"

#define DEBUG 1

/* define to avoid compilation warning */
#define LWIP_TIMEVAL_PRIVATE 0

#define MAIN_TASK_LOOP_TIME 1 //milliseconds

#define PARKING_BRAKE_DUTY_CYCLE 0.25
#define COME_TO_STOP_BRAKE_DUTY_CYCLE 0.5
#define EMERGENCY_STOP_BRAKE_DUTY_CYCLE 1.0

#define  MAX_STEERING_ENCODER_COUNT 350
#define  MIN_STEERING_ENCODER_COUNT -350
#define MAX_STEERING_ANGLE 50.0
#define MIN_STEERING_ANGLE -50.0

#define MAX_VEHICLE_SPEED 5.0
#define MIN_VEHICLE_SPEED -1.0

#define MAX_STEERING_DUTY_CYCLE 1.0
#define MAX_ACCEL_DUTY_CYCLE 1.0
#define MIN_ACCEL_DUTY_CYCLE -0.25

//duty cycle (0.0 - 1.0) / degrees
#define STEERING_P_GAIN (1.0 / 30.0) //100% duty cycle at angles greater than 60 deg
//duty cycle / degrees
#define STEERING_I_GAIN 0 //5% duty cycle for every second we are 1 deg off. // Samer: Changed to zero for MPC control
#define STEERING_D_GAIN 0.0

//duty cycle (0.0 - 1.0) / m/s
#define SPEED_P_GAIN (1.0 / 1.0) //100% duty cycle at speed errors > 2.2 MPH
#define SPEED_I_GAIN 0 //5% duty cycle for every second we are .2 MPH off of our target.  // Samer: Changed to zero for MPC control
#define SPEED_D_GAIN 0.0

// Variables for use in steering encoder ISR
volatile int aFlag = 0;
volatile int bFlag = 0;
volatile int counter = 0;
volatile int last_count = 0;


static main_context_t ctx;
volatile encoder_variables steering_encoder_vars;


#ifdef DEBUG
void print_ipaddress(void)
{
	static char tmp_buff[16];
	printf("IP_ADDR    : %s\r\n",
	       ipaddr_ntoa_r((const ip_addr_t *)&(TCPIP_STACK_INTERFACE_0_desc.ip_addr), tmp_buff, 16));
	printf("NET_MASK   : %s\r\n",
	       ipaddr_ntoa_r((const ip_addr_t *)&(TCPIP_STACK_INTERFACE_0_desc.netmask), tmp_buff, 16));
	printf("GATEWAY_IP : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *)&(TCPIP_STACK_INTERFACE_0_desc.gw), tmp_buff, 16));
}
#endif

uint32_t GetCurrentTime()
{
	return xTaskGetTickCount();
}
int ConvertAngleToPIDInt(float angle)
{
	return (int)(angle * 1000.0);
}
int ConvertSpeedToPIDInt(float speed)
{
	return (int)(speed * 1000.0);
}
float ConvertPIDIntToDutyCycle(int PID_int)
{
	return ((float)PID_int) / 1000.0;
}
int ConvertDutyCycleToPIDInt(float duty_cycle)
{
	return ((int)duty_cycle) * 1000.0;
}
//PID Callback functions
int SteeringPIDSource()
{
	return ConvertAngleToPIDInt(ctx.steering_angle_commanded);
}
int SpeedPIDSource()
{
	return ConvertSpeedToPIDInt(ctx.vehicle_speed_commanded);
}
void SteeringPIDOutput(int pid_output)
{
	ctx.steering_torque_pid_out = ConvertPIDIntToDutyCycle(pid_output);
}
void SpeedPIDOutput(int pid_output)
{
	ctx.acceleration_pid_out = ConvertPIDIntToDutyCycle(pid_output);
}
unsigned long GetPIDTime()
{
	return (unsigned long) GetCurrentTime();
}
void OverridePID()
{
	if( !ctx.override_pid )
		return;

	ctx.speed_controller.p = ctx.speed_p_gain_override;
	ctx.speed_controller.i = ctx.speed_i_gain_override;
	ctx.speed_controller.d = ctx.speed_d_gain_override;

	ctx.steering_controller.p = ctx.steer_p_gain_override;
	ctx.steering_controller.i = ctx.steer_i_gain_override;
	ctx.steering_controller.d = ctx.steer_d_gain_override;
}


void ProcessAlgorithms(main_context_t* ctx)
{
	ctx->estop_indicator = 0;
	OverridePID();
	setEnabled(&ctx->steering_controller, ctx->autonomous_mode && !ctx->estop_in && !ctx->park_brake_commanded);
	setEnabled(&ctx->speed_controller, ctx->autonomous_mode && !ctx->estop_in && !ctx->park_brake_commanded);
	tick(&ctx->steering_controller);
	tick(&ctx->speed_controller);

	if( ctx->last_eth_input_rx_time - ctx->current_time > 250)
	{
		ctx->pc_comm_active = 0;
		ctx->autonomous_mode = 0;
	}
	
	if(ctx->autonomous_mode)
	{
		SetEStopState(ctx->estop_in);
		SetSafetyLight1On(1);
		
		if(ctx->estop_in)
		{
			SetFrontBrake(EMERGENCY_STOP_BRAKE_DUTY_CYCLE);
			SetAcceleration(0.0);
			ctx->estop_indicator = 1;
		}
		else if(ctx->park_brake_commanded)
		{
			SetFrontBrake(PARKING_BRAKE_DUTY_CYCLE);
			SetAcceleration(0.0);
		}
		else
		{
			//regular autonomous mode
			float accel = ctx->acceleration_pid_out;
			
			//if reverse commanded
			if( accel < 0.0 )
			{
				//if we are moving forward
				if(ctx->vehicle_speed > 0)
				{
					//Don't engage reverse while we are moving forward.
					accel = 0.0;
					SetFrontBrake(COME_TO_STOP_BRAKE_DUTY_CYCLE);
				}
				else //not moving forward and reverse commanded
				{
					SetReverseDrive(1);
					accel = -accel;
				}
			}
			else //reverse not commanded
			{
				SetReverseDrive(0);
			}
			if( accel > 1.0)
				accel = 1.0;
			SetAcceleration( accel );

			float SteeringTorqueFromPID = ctx->steering_torque_pid_out;
			SetSteerDirection(SteeringTorqueFromPID < 0.0);

			//limit the torque 0 to 1
			if( SteeringTorqueFromPID < 0.0 )
				SteeringTorqueFromPID = -SteeringTorqueFromPID;
			if( SteeringTorqueFromPID > 1.0 )
				SteeringTorqueFromPID = 1.0;

			SetSteeringTorque(SteeringTorqueFromPID);
		}
	}
	else //not in autonomous mode
	{
		SetSafetyLight1On(0);
		SetSteeringTorque(0.0);
		SetAcceleration(0.0);
		SetReverseDrive(0);
	}
}
uint32_t last_test_tick = 0;
int tick_tock = 0;
void TestSystems(main_context_t* ctx)
{
	if( ctx->current_time - last_test_tick > 2000)
	{
		last_test_tick = ctx->current_time;
		tick_tock = !tick_tock;
		
		//alternate these signals
		if(tick_tock)
		{
			SetAcceleration(0.40);
			SetFrontBrake(0.0);
			SetReverseDrive(0);
			SetSteerDirection(0);
		}
		else
		{
			SetAcceleration(0.40);
			SetReverseDrive(1);
			SetFrontBrake(0.2);
			SetSteerDirection(1);
		}
		SetSafetyLight1On(1);
		SetSteeringTorque(0.40);
	}
	
}

void TeleOperation(main_context_t* ctx)
{
	if(ctx->tele_operation_enabled && ctx->current_time - ctx->last_eth_input_rx_time < 100)
	{
		SetSafetyLight1On(1);
		SetAcceleration(ctx->vehicle_speed_commanded);
		if(ctx->steering_angle_commanded > 0)
		{
			SetSteeringTorque(ctx->steering_angle_commanded);
			SetSteerDirection(0);
		}
		else
		{
			SetSteeringTorque(ctx->steering_angle_commanded * -1);
			SetSteerDirection(1);
		}
		
	}
	else
	{
		SetAcceleration(0.0);
		SetSafetyLight1On(0);
	}
}


void main_task(void* p)
{
	main_context_t* context = (main_context_t*)p; 
	
	while (1)
	{
		// Here we check and see if we may gain access to the shared context resource through he xSemaphoreTake function
		if(xSemaphoreTake(context->sem, 50) != pdTRUE)
		{
			printf("Failed to take IO semaphore!\n");
			vTaskDelay(1);
		}
		
		context->current_time = GetCurrentTime();
		TeleOperation(context);
		//set_steering_angle(context, &(steering_encoder_vars.counter)); 
		//debug_collector(context);
		/*ProcessCurrentInputs(context);
		ProcessAlgorithms(context);
		ProcessCurrentOutputs(context);*/
		//TestSystems(context);
		
		//Here we release access to the semaphore to be used in the ethenet_thread task
		xSemaphoreGive(context->sem);
		
		//We add a delay to allow some time for the ethernet_thread task to gain access to the semaphore, other wise the while loop would immediately begin and take access (I think this is right)
		vTaskDelay(MAIN_TASK_LOOP_TIME);
	}
}


void count_encoderA (void)
{	
	ext_irq_disable(encoder_A);
	ext_irq_disable(encoder_B);
	
	BaseType_t task_woken =pdFALSE;
	
	int encoderA_state = gpio_get_pin_level(encoder_A);
	int encoderB_state = gpio_get_pin_level(encoder_B);
	
	if (encoderA_state == 1 && encoderB_state == 1 && aFlag == 1){
		
		--steering_encoder_vars.counter;
		steering_encoder_vars.aFlag = 0;
		steering_encoder_vars.bFlag = 0;
		
		//printf("%d ", steering_encoder_vars.counter);
	}
	else if(encoderB_state == 1){
		bFlag = 1;
	}

	
	xSemaphoreGiveFromISR(ctx.sem, &task_woken);
	
	
	if (task_woken == pdTRUE)
	portYIELD_FROM_ISR( task_woken);
	ext_irq_enable(encoder_A);
	ext_irq_enable(encoder_B);

}

void count_encoderB (void)
{	
	BaseType_t task_woken = pdFALSE;
	
	ext_irq_disable(encoder_A);
	ext_irq_disable(encoder_B);
	
	int encoderA_state = gpio_get_pin_level(encoder_A);
	int encoderB_state = gpio_get_pin_level(encoder_B);
	
	if (encoderA_state == 1 && encoderB_state == 1 && bFlag == 1){
		
		++steering_encoder_vars.counter;
		steering_encoder_vars.aFlag=0;
		steering_encoder_vars.bFlag=0;
		printf("%d ", steering_encoder_vars.counter);
		
	}
	else if(encoderA_state == 1){
		aFlag = 1;
	}
	
	
	xSemaphoreGiveFromISR(ctx.sem, &task_woken);
	
	//portEND_SWITCHING_ISR(task_woken);
	if (task_woken == pdTRUE)
	portYIELD_FROM_ISR(task_woken);
	
	ext_irq_enable(encoder_A);
	ext_irq_enable(encoder_B);
	
}



int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	
	ctx.steeringangle_to_encoder_ratio = (MAX_STEERING_ANGLE - MIN_STEERING_ANGLE) / (float)(MAX_STEERING_ENCODER_COUNT - MIN_STEERING_ENCODER_COUNT);
	ctx.steering_controller.p = STEERING_P_GAIN;
	ctx.steering_controller.i = STEERING_I_GAIN;
	ctx.steering_controller.d = STEERING_D_GAIN;
	ctx.steering_controller.pidSource = SteeringPIDSource;
	ctx.steering_controller.pidOutput = SteeringPIDOutput;
	setInputBounds(&(ctx.steering_controller), ConvertAngleToPIDInt(MIN_STEERING_ANGLE), ConvertAngleToPIDInt(MAX_STEERING_ANGLE));
	setOutputBounds(&(ctx.steering_controller), ConvertDutyCycleToPIDInt(MAX_STEERING_DUTY_CYCLE)*-1, ConvertDutyCycleToPIDInt(MAX_STEERING_DUTY_CYCLE));
	ctx.steering_controller.getSystemTime = GetPIDTime;

	ctx.speed_controller.p = SPEED_P_GAIN;
	ctx.speed_controller.i = SPEED_I_GAIN;
	ctx.speed_controller.d = SPEED_D_GAIN;
	ctx.speed_controller.pidSource = SpeedPIDSource;
	ctx.speed_controller.pidOutput = SpeedPIDOutput;
	setInputBounds(&(ctx.speed_controller), ConvertSpeedToPIDInt(MIN_VEHICLE_SPEED), ConvertSpeedToPIDInt(MAX_VEHICLE_SPEED));
	setOutputBounds(&(ctx.speed_controller), ConvertDutyCycleToPIDInt(MIN_ACCEL_DUTY_CYCLE), ConvertDutyCycleToPIDInt(MAX_ACCEL_DUTY_CYCLE));
	ctx.speed_controller.getSystemTime = GetPIDTime;
	
	//Initialize Encoder 
	steering_encoder_vars.aFlag=0;
	steering_encoder_vars.bFlag=0;
	steering_encoder_vars.counter=0;
	steering_encoder_vars.last_count=0;

	
	memset(&ctx, 0, sizeof(ctx));
	
	

	// The idea of a Semaphore is to manage access to a shared resource, in our case its context ctx, which holds the vehicle information
	// Context is shared between the 1-ethernet_thread task, which handles communications with the host computer and 2-main_task which controls the vehicle's functionality
	
	// ctx.sem refers to the handle semaphore which acts as the gatekeeper of the shared resource, a Binary Semaphore allows the shared resource to be occupied
	// by one task at a time
	ctx.sem = xSemaphoreCreateBinary();
	
	// We make the semaphore available
	xSemaphoreGive(ctx.sem);
	
	// Interrupt priorities for encoder pins. (lowest value = highest priority) 
	// ISRs using FreeRTOS *FromISR APIs must have priorities below or equal to 
	// configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY. This version of RTOS, it is 4
	NVIC_SetPriority(EIC_0_IRQn, 5);
	NVIC_SetPriority(EIC_7_IRQn, 5);	
	// Register the Interrupts and assign the pins to the functions
	ext_irq_register(PIN_PB07, count_encoderA);
	ext_irq_register(PIN_PD00, count_encoderB);

	xTaskCreate(ethernet_thread,
		"Ethernet_Task",
		2048,
		&ctx,
		0,
		NULL);

	xTaskCreate(main_task,
		"Main_Task",
		2048,
		&ctx,
		2,
		NULL);	

	vTaskStartScheduler();
	
	//Should never reach here as vTaskStartScheduler is infinitely blocking
	//If we DO reach here then there is insufficient RAM
	printf("Insufficient RTOS heap available to create the idle or timer daemon tasks");

	return 0;
}
