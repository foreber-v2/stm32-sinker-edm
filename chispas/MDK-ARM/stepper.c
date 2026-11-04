#include "global_vars.h"
#include "stepper.h"
#include "stm32f1xx_ll_tim.h"
#include "tim.h"

static volatile uint8_t step_busy = 0;
static volatile uint32_t planned_steps = 0;	// num de pasos objetivo
static volatile uint8_t step_state = 0;	 		// high  o low
static volatile uint8_t direction_paso = 0;	  // antihorario u horario

static void step_low(void)          { LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_2); }
static void step_high(void)         { LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_2); }
static void dir_low(void)           { LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_10); }
static void dir_high(void)          { LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_10); }

// prueba nuevo par de steppers segundo eje 
// ya configurada la direction, sube sube, baja baja
//estos son los pines para el driver nuevo
//static void step_low(void)          { LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0); }
//static void step_high(void)         { LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_0); }
//static void dir_high(void)           { LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7); }
//static void dir_low(void)          { LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_7); }



static void Stepper_timer_run(uint32_t consigna)
{
	LL_TIM_DisableCounter(TIM2);
	LL_TIM_ClearFlag_UPDATE(TIM2);
	LL_TIM_SetCounter(TIM2, 0);
	LL_TIM_SetAutoReload(TIM2, TIME_BETWEEN_STEPS);
	LL_TIM_EnableCounter(TIM2);
}

/* -- API publica ------------------------------------------------------- */

void Stepper_Init(void)
{
	LL_TIM_SetAutoReload(TIM2, 65535);
	LL_TIM_ClearFlag_UPDATE(TIM2);
	LL_TIM_EnableIT_UPDATE(TIM2);
	
	NVIC_SetPriority(TIM2_IRQn, 5);
  NVIC_EnableIRQ(TIM2_IRQn);
	
	Stepper_Stop();
}


void Stepper_Stop(void)
{
	LL_TIM_DisableCounter(TIM2);
	LL_TIM_ClearFlag_UPDATE(TIM2);
	step_low();
	step_state = 0;
	completed_steps = 0;
	step_busy = 0;
}

uint8_t Stepper_IsBusy(void)
{
	return step_busy;
}

uint8_t Stepper_Aprox_Inicial(uint8_t direction)
{
	if (step_busy) return 1;
	step_busy = 1;
	planned_steps = NUM_APROX_STEPS;
	if (direction)
	{
		dir_high();
	}
	else
	{
		dir_low();
	}
	
	Stepper_timer_run(TIME_BETWEEN_STEPS);
	
	return 0;
}

uint8_t Stepper_Mov_Rapido(uint8_t direction)
{
	if (step_busy) return 1;
	step_busy = 1;
	planned_steps = NUM_QUICK_STEPS;
	if (direction)
	{
		dir_high();
	}
	else
	{
		dir_low();
	}
	
	Stepper_timer_run(TIME_BETWEEN_STEPS);
	
	return 0;
}

uint8_t Stepper_Mov_Mid(uint8_t direction)
{
	if (step_busy) return 1;
	step_busy = 1;
	planned_steps = NUM_STEPS_MID;
	if (direction)
	{
		dir_high();
	}
	else
	{
		dir_low();
	}
	
	Stepper_timer_run(TIME_BETWEEN_STEPS);
	
	return 0;
}

void Stepper_IRQHandler(void)
{
		if (LL_TIM_IsActiveFlag_UPDATE(TIM2)) 
    {
			// Limpiar el flag inmediatamente
			LL_TIM_ClearFlag_UPDATE(TIM2); 
			
			if (completed_steps < planned_steps)
			{
				if (step_state == 0)
				{
					step_high();
					step_state = 1;
					completed_steps++;
				}
				else
				{
					step_low();
					step_state = 0;
				}
			}
			else
			{
				step_low();
				Stepper_Stop();
			}
    }
		
}

