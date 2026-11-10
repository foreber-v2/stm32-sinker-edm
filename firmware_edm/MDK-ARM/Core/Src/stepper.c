/**
 ******************************************************************************
 * @file    stepper.c
 * @brief   Stepper motor driver: STEP pulses generated from the TIM2 update IRQ
 ******************************************************************************
 *
 * INTERNAL STATE MACHINE (full diagram in stepper_fsm.mermaid)
 *
 *   IDLE --Stepper_Aprox_Inicial / Mov_Mid / Mov_Rapido--> STEP_LOW (waiting)
 *   STEP_LOW  --TIM2 update, completed_steps < planned_steps--> STEP_HIGH (completed_steps++)
 *   STEP_HIGH --TIM2 update, completed_steps < planned_steps--> STEP_LOW
 *   STEP_HIGH --TIM2 update, completed_steps = planned_steps--> IDLE (Stepper_Stop)
 *   any state --Stepper_Stop()--> IDLE
 *
 * PIN MAP
 *   PA2   STEP
 *   PB10  DIR   (direction argument 0 = BAJA -> DIR low, 1 = SUBE -> DIR high)
 *
 * NOTE: "completed_steps" is a global declared in global_vars.h.
 ******************************************************************************
 */

#include "global_vars.h"
#include "stepper.h"
#include "stm32f1xx_ll_tim.h"
#include "tim.h"

/* -- Private variables ------------------------------------------------------ */
static volatile uint8_t  step_busy      = 0;   /* 1 = move in progress                     */
static volatile uint32_t planned_steps  = 0;   /* Target number of steps                   */
static volatile uint8_t  step_state     = 0;   /* Current STEP pin phase: 0 = low, 1 = high */
static volatile uint8_t  direction_paso = 0;   /* Direction (counter-clockwise / clockwise); not used in this file */

/* -- Pin helpers ------------------------------------------------------------ */
/* Current driver: STEP = PA2, DIR = PB10 */
static void step_low(void)          { LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_2); }
static void step_high(void)         { LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_2); }
static void dir_low(void)           { LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_10); }
static void dir_high(void)          { LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_10); }

/* TEST / ALTERNATIVE (not needed for normal operation):
 * Pins for the new driver on the second axis (STEP = PB0, DIR = PA7).
 * Direction is already configured so that "up goes up, down goes down"
 * (dir_high / dir_low are swapped on purpose in this version).
 * To use it, comment out the helpers above and enable these.
 *
 *   static void step_low(void)       { LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0); }
 *   static void step_high(void)      { LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_0); }
 *   static void dir_high(void)       { LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7); }
 *   static void dir_low(void)        { LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_7); }
 */

/* -- Private functions ------------------------------------------------------ */

/**
 * @brief  Starts TIM2 so the update interrupt fires every TIME_BETWEEN_STEPS ticks.
 * @param  consigna  Not used: the period is always TIME_BETWEEN_STEPS.
 */
static void Stepper_timer_run(uint32_t consigna)
{
    LL_TIM_DisableCounter(TIM2);
    LL_TIM_ClearFlag_UPDATE(TIM2);
    LL_TIM_SetCounter(TIM2, 0);
    LL_TIM_SetAutoReload(TIM2, TIME_BETWEEN_STEPS);
    LL_TIM_EnableCounter(TIM2);
}

/* -- Public API ------------------------------------------------------------- */

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
    step_state      = 0;
    completed_steps = 0;
    step_busy       = 0;
}

uint8_t Stepper_IsBusy(void)
{
    return step_busy;
}

/* Fine approach move (NUM_APROX_STEPS). Returns 1 if busy, 0 if started. */
uint8_t Stepper_Aprox_Inicial(uint8_t direction)
{
    if (step_busy) return 0;

    step_busy     = 1;
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

    return 1;
}

/* Fast move (NUM_QUICK_STEPS). Returns 1 if busy, 0 if started. */
uint8_t Stepper_Mov_Rapido(uint8_t direction)
{
    if (step_busy) return 0;

    step_busy     = 1;
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

    return 1;
}

/* Medium move (NUM_STEPS_MID). Returns 1 if busy, 0 if started. */
uint8_t Stepper_Mov_Mid(uint8_t direction)
{
    if (step_busy) return 0;

    step_busy     = 1;
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

    return 1;
}

/* -- Interrupt handler (call from TIM2_IRQHandler) --------------------------- */
void Stepper_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM2))
    {
        /* Clear the flag immediately */
        LL_TIM_ClearFlag_UPDATE(TIM2);

        if (completed_steps < planned_steps)
        {
            /* Still steps to do: toggle the STEP pin */
            if (step_state == 0)
            {
                /* Rising edge: this counts as one step */
                step_high();
                step_state = 1;
                completed_steps++;
            }
            else
            {
                /* Falling edge: end of the pulse */
                step_low();
                step_state = 0;
            }
        }
        else
        {
            /* Target reached: the last pulse ends here, then stop everything */
            step_low();
            Stepper_Stop();
        }
    }
}