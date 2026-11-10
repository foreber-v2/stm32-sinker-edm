/**
 ******************************************************************************
 * @file    stepper.h
 * @brief   Stepper motor driver (STEP / DIR pulses generated from TIM2 update IRQ)
 *          Blue Pill STM32F103C8 - LL drivers
 ******************************************************************************
 *
 * HOW IT WORKS
 *   - TIM2 raises an update interrupt every TIME_BETWEEN_STEPS ticks.
 *   - Each interrupt toggles the STEP pin: HIGH, then LOW, then HIGH, ...
 *   - One full step = one HIGH phase + one LOW phase = 2 interrupts.
 *   - The move ends when the requested number of steps has been generated.
 *
 *   STEP  ____|?????|_____|?????|_____|?????|_____
 *             ^     ^     ^
 *             IRQ   IRQ   IRQ ... (every TIME_BETWEEN_STEPS ticks)
 *
 * TIMING (TIM2 prescaler = 35 in tim.c, assuming a 72 MHz timer clock -> 0.5 us/tick)
 *   TIME_BETWEEN_STEPS = 300 ticks -> about 150 us per phase, about 300 us per step
 *   (about 3.3 kHz step rate).
 *
 * PINS (see stepper.c)
 *   PA2   STEP
 *   PB10  DIR   (BAJA = DIR low, SUBE = DIR high)
 ******************************************************************************
 */

#ifndef STEPPER_H
#define STEPPER_H

#include "stm32f1xx.h"
#include <stdint.h>

/* -- Move lengths (in steps) and timing -------------------------------------- */
#define NUM_APROX_STEPS         1       /* Fine approach move: 1 step                  */
#define NUM_STEPS_MID           50      /* Medium move: 50 steps                       */
#define NUM_QUICK_STEPS         35000   /* Fast move: 35000 steps                      */
#define TIME_BETWEEN_STEPS      300u    /* TIM2 auto-reload value = time per phase     */

/* -- Public API -------------------------------------------------------------- */

/**
 * @brief  Initializes TIM2 for the step generation.
 *         Call ONCE after MX_TIM2_Init().
 *         Leaves the timer ready but not generating steps.
 */
void Stepper_Init(void);

/**
 * @brief  Starts a fine approach move (NUM_APROX_STEPS steps).
 *         Does nothing if a move is already in progress.
 * @param  direction  BAJA (0) or SUBE (1)
 * @retval 1 if the move was started, 0 if the driver was busy
 */
uint8_t Stepper_Aprox_Inicial(uint8_t direction);

/**
 * @brief  Starts a medium move (NUM_STEPS_MID steps).
 *         Does nothing if a move is already in progress.
 * @param  direction  BAJA (0) or SUBE (1)
 * @retval 1 if the move was started, 0 if the driver was busy
 */
uint8_t Stepper_Mov_Mid(uint8_t direction);

/**
 * @brief  Starts a fast move (NUM_QUICK_STEPS steps).
 *         Does nothing if a move is already in progress.
 * @param  direction  BAJA (0) or SUBE (1)
 * @retval 1 if the move was started, 0 if the driver was busy
 */
uint8_t Stepper_Mov_Rapido(uint8_t direction);

/**
 * @brief  Stops any move: disables TIM2, sets STEP low and resets the
 *         internal counters. Also called automatically when a move ends.
 */
void Stepper_Stop(void);

/**
 * @brief  Checks whether a move is in progress.
 * @retval 1 = busy, 0 = free
 */
uint8_t Stepper_IsBusy(void);

/**
 * @brief  Must be called from TIM2_IRQHandler.
 *         Advances the internal step-generation state machine.
 */
void Stepper_IRQHandler(void);

/* NOTE: Stepper_timer_run() is a private static function defined in stepper.c.
 * Static prototypes do not belong in a header, so it was removed from here. */

#endif /* STEPPER_H */