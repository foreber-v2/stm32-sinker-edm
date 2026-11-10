/* discharge.h
 * Driver para ciclo de descarga via TIM1 CH1/CH1N/CH2
 * Blue Pill STM32F103C8 - LL drivers

 * TIM1: Prescaler=71 → 1 tick = 1us a 72MHz
 *       Modo repetitivo con IRQ de Update (UIF) al fin de cada tramo
 *       Deadtime BDTR=35 ticks a 72MHz ≈ 486ns ≈ 500ns
 */

#ifndef DISCHARGE_H
#define DISCHARGE_H

#include "stm32f1xx.h"
#include <stdint.h>

#define T_TOTAL_US       60u   // ARR: duración total del pulso
#define T_CH1_TO_LOW_US    23u    // CCR1: CH1 HIGH antes de este tick
#define T_CH2_TO_HIGH_US   11u   // CCR2: CH2 HIGH desde este tick
#define T_CH2_TO_LOW_US    50u   // CCR4: CH2 LOW desde este tick
//#define CRR4_PWMN2_US  50u   // CCR3: Alto total

#define NUM_PULSES 			 8		 //Cantidad de pulsos antes de parar


/* ── API publica ─────────────────────────────────────────────────────── */

/**
 * @brief  Inicializa y ajusta TIM1 para el ciclo de descarga.
 *         Llamar UNA VEZ despues de MX_TIM1_Init().
 *         Deja el timer listo pero sin disparar.
 */
void Discharge_Init(void);

/**
 * @brief  Dispara un ciclo de descarga unico.
 *         No hace nada si ya hay un ciclo en curso.
 * @retval 1 si el ciclo fue disparado, 0 si estaba ocupado
 */
uint8_t Discharge_RunCycle_Continuo(void);

/**
 * @brief  Consulta si hay un ciclo en curso.
 * @retval 1 = ocupado, 0 = libre
 */
uint8_t Discharge_RunCycle_Once(void);

/**
 * @brief  Consulta si hay un ciclo en curso.
 * @retval 1 = ocupado, 0 = libre
 */
uint8_t Discharge_RunCycle_Train(void);

/**
 * @brief  Consulta si hay un ciclo en curso.
 * @retval 1 = ocupado, 0 = libre
 */
uint8_t Discharge_IsBusy(void);

/**
 * @brief  Llamar desde TIM1_UP_IRQHandler.
 *         Avanza la FSM interna del ciclo de descarga.
 */
void Discharge_IRQHandler(void);

#endif /* DISCHARGE_H */
