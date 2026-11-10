/**
 ******************************************************************************
 * @file    discharge.h
 * @brief   Discharge cycle driver using TIM1 CH1 / CH1N / CH2 (+ CC3, CC4 events)
 *          Blue Pill STM32F103C8 - LL drivers
 ******************************************************************************
 *
 * TIMER SETUP (configured in tim.c, MX_TIM1_Init)
 *   - Prescaler = 71  -> 1 tick = 1 us at 72 MHz
 *   - Dead-time (BDTR DeadTime) is set in tim.c; check the value there.
 *   - Only the Capture/Compare interrupt (TIM1_CC_IRQn) is used.
 *     The Update interrupt (UIF) is NOT used.
 *
 * PULSE SHAPE (one pulse, 1 tick = 1 us, values from the defines below)
 *
 *   tick    0          11         23                  50       59  60
 *   CH1     ############################|______________________________   PWM1: HIGH while CNT < CCR1
 *   CH1N    (complementary of CH1, with dead-time)
 *   CH2     ___________|##########################|____________________   HIGH at CCR2, forced LOW at CC4
 *   CC3                                                        ^         end-of-pulse event (ARR - 1)
 *
 *   CH1 / CH1N : PWM1 mode, HIGH from tick 0 up to T_CH1_TO_LOW_US
 *   CH2        : "Active on match" mode, goes HIGH at T_CH2_TO_HIGH_US and is
 *                forced LOW by the ISR at T_CH2_TO_LOW_US (CC4 event)
 *   CC3        : fires at T_TOTAL_US - 1, the ISR stops the timer or re-arms CH2
 ******************************************************************************
 */

#ifndef DISCHARGE_H
#define DISCHARGE_H

#include "stm32f1xx.h"
#include <stdint.h>

/* -- Pulse timing (in timer ticks, 1 tick = 1 us) ---------------------------- */
#define T_TOTAL_US          60u     /* ARR : total pulse duration                    */
#define T_CH1_TO_LOW_US     23u     /* CCR1: CH1 is HIGH before this tick            */
#define T_CH2_TO_HIGH_US    11u     /* CCR2: CH2 goes HIGH at this tick              */
#define T_CH2_TO_LOW_US     50u     /* CCR4: CH2 is forced LOW at this tick          */
//#define CRR4_PWMN2_US     50u     /* (unused, old CCR3/CCR4 naming kept for reference) */

/* -- Pulse train ------------------------------------------------------------- */
#define NUM_PULSES          8       /* Number of pulses in a train before stopping   */

/* -- Public API -------------------------------------------------------------- */

/**
 * @brief  Initializes and configures TIM1 for the discharge cycle.
 *         Call ONCE after MX_TIM1_Init().
 *         Leaves the timer ready but not firing, outputs in idle state.
 */
void Discharge_Init(void);

/**
 * @brief  Declared for a continuous discharge cycle.
 * @note   NOT implemented in discharge.c. Remove it, or implement it.
 */
uint8_t Discharge_RunCycle_Continuo(void);

/**
 * @brief  Fires a single discharge pulse.
 *         Does nothing if a cycle is already running.
 * @retval 1 if the pulse was fired, 0 if the driver was busy
 */
uint8_t Discharge_RunCycle_Once(void);

/**
 * @brief  Fires a train of NUM_PULSES discharge pulses.
 *         Does nothing if a cycle is already running.
 * @retval 1 (see note in discharge.c about the busy case)
 */
uint8_t Discharge_RunCycle_Train(void);

/**
 * @brief  Checks whether a cycle is running.
 * @retval 1 = busy, 0 = free
 */
uint8_t Discharge_IsBusy(void);

/**
 * @brief  Must be called from TIM1_CC_IRQHandler.
 *         Advances the internal discharge state machine (CC4 and CC3 events).
 */
void Discharge_IRQHandler(void);

#endif /* DISCHARGE_H */