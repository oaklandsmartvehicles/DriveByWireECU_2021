/*
 * DriveByWireIO.c
 *
 * Created: 9/29/2019 4:01:47 PM
 *  Author: John Brooks
 */ 
#include "DriveByWireIO.h"
#include "hal_gpio.h"
#include "hal_delay.h"
#include "hal_usart_sync.h"
#include "hal_pwm.h"
#include "driver_init.h"
#include "main_context.h"

//PWM clock is 12Mhz
#define PWM_TICKS_PER_SECOND 0xB71B00

//PWM Frequency in Hz
#define VEHICLE_SPEED_FREQ 1000
#define STEERING_TORQUE_FREQ 30000
#define FRONT_BRAKE_FREQ 1000
#define REAR_BRAKE_FREQ 1000
#define ACCELERATION_FREQ 1000

//Duty Cycle of Rear Brake
#define REAR_BRAKE_ENGAGED_DUTY_CYCLE 0.50f
#define REAR_BRAKE_DISENGAGED_DUTY_CYCLE 0.0f

//Mapping of PWM to Outputs
#define PWM_Acceleration ACCELERATION		//PA05
#define PWM_SteeringTorque STEERINGPOWER	//PB09
#define PWM_FrontBrake BRAKE		//PB15
//#define PWM_RearBrake PWM_3			//PB03

//rename this if used for something. For now PWM_4 is just an available pin.
#define PWM_ExtraPWM PWM_4

typedef union adc_val_t {
	uint8_t ADC_Read_8[2];
	uint16_t ADC_Read;
} adc_val_t;


 
 

float LinearlyInterpolate(float val_x, float left_x, float right_x, float left_y, float right_y)
{
	float delta_x = right_x - left_x;
	float percent_x = (val_x - left_x) / delta_x;
	return left_y + (percent_x * (right_y - left_y));
}


// TODO: Determine mapping of motor encoder to steering angle and use interpolation utility function above for interpreting the result
void set_steering_angle(main_context_t* ctx, const volatile int* encoder_count)
{
	ctx->steering_angle = (*encoder_count)*ctx->steeringangle_to_encoder_ratio;
}


void ProcessCurrentInputs(main_context_t* context)
{
	context->estop_in = !gpio_get_pin_level(EStop_In);
	//context->steering_angle = ReadSteeringPosition();
}

void ProcessCurrentOutputs(main_context_t* context)
{	
	SetPCComm(context->pc_comm_active);
}

//non-zero values turn lights on
void SetSafetyLight1On(int on)
{
	gpio_set_pin_level(SafetyLights1Enable, on);
}
void SetSafetyLight2On(int on)
{
	gpio_set_pin_level(SafetyLights2Enable, on);
}

//non zero values steer right, zero steers left.
void SetSteerDirection(int right)
{
	gpio_set_pin_level(SteeringDirection, right);
}

//Puts the vehicle in reverse if value is non-zero.
void SetReverseDrive(int reverse)
{
	gpio_set_pin_level(Reverse, reverse);
	gpio_set_pin_level(NotReverse, !reverse);
	SetSafetyLight2On(reverse);
}

//Applies power to the steering motor as duty cycle percentage
void SetSteeringTorque(float duty_cycle)
{
	if(duty_cycle < 0)
		duty_cycle = 0;
	else if (duty_cycle > 1.0)
		duty_cycle = 1;
	
	duty_cycle = 1 - duty_cycle;
	
	duty_cycle = duty_cycle * 0.6;
		
	const static int STEERING_TORQUE_FREQ_TICKS = PWM_TICKS_PER_SECOND / STEERING_TORQUE_FREQ;
	pwm_set_parameters(&PWM_SteeringTorque, STEERING_TORQUE_FREQ_TICKS, duty_cycle * STEERING_TORQUE_FREQ_TICKS);
	pwm_enable(&PWM_SteeringTorque);

	gpio_set_pin_level(SteeringEnable, duty_cycle > 0.0);
}

 //Sets the front brake PWM as duty cycle percentage.
 void SetFrontBrake(float duty_cycle)
 {
	const static int FRONT_BRAKE_FREQ_TICKS = PWM_TICKS_PER_SECOND / FRONT_BRAKE_FREQ;
	pwm_set_parameters(&PWM_FrontBrake, FRONT_BRAKE_FREQ_TICKS, duty_cycle * FRONT_BRAKE_FREQ_TICKS);
	pwm_enable(&PWM_FrontBrake);
 }

//Sets the acceleration value to the specified duty cycle
void SetAcceleration(float duty_cycle)
{	
	const static int ACCELERATION_FREQ_TICKS = PWM_TICKS_PER_SECOND / ACCELERATION_FREQ;
	pwm_set_parameters(&PWM_Acceleration, ACCELERATION_FREQ_TICKS, duty_cycle * ACCELERATION_FREQ_TICKS);
	pwm_enable(&PWM_Acceleration);

	if(duty_cycle > 0)
	{
		gpio_set_pin_level(AccelerationEnable, 1);
	}
	else
	{
		gpio_set_pin_level(AccelerationEnable, 0);
	}
}

//Non-zero values turns the PC Comm LED ON.
void SetPCComm(int active)
{
	gpio_set_pin_level(PCComm, active);
}

//non-zero values turns the EStop LED ON.
void SetEStopState(int active)
{
	gpio_set_pin_level(EStopState, active);
}

void debug_collector(main_context_t* context)
{
	SetDebugLED1(context->debug_led_1);
	SetDebugLED2(context->debug_led_1);
	SetEStopState(context->estop_indicator);
	SetDebugSteeringAngle(context->steering_angle);
	SetDebugVehicleSpeed(context->vehicle_speed);
}
//non-zero values turns on the debug LEDs.
void SetDebugLED1(int active)
{
	gpio_set_pin_level(LED1, active);
}
void SetDebugLED2(int active)
{
	gpio_set_pin_level(LED2, active);
}

// Print the Steering Angle
void SetDebugSteeringAngle(float steering_angle)
{
	printf("%f ", steering_angle);
}

//Print the
void SetDebugVehicleSpeed(float longitudinal_speed)
{
	printf("%f ", longitudinal_speed);
}