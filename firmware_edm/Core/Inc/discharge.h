/* discharge.h
 * Driver para ciclo de descarga via TIM1 CH1/CH1N/CH2
 * Blue Pill STM32F103C8 - LL drivers
 *
 * Secuencia de un ciclo (tiempos aproximados, deadtime ~0.5us en hardware):
 *
 *  t=0        t=100    t=100.5  t=102.5  t=130.5        t=202.5
 *  |            |        |        |          |               |
 *  CH1N: [=100us=]      [dt]     [====100us (desde t=102.5)====]LOW
 *  CH1:          [dt]   [=2us=]LOW
 *  CH2:                 [=========30us=========]LOW
 *
 * Tramos del codigo:
 *   TRAMO 1: CH1N solo,   100us  ARR=99,  CCR1=100 (CCR1>ARR → CH1N siempre HIGH)
 *   TRAMO 2: CH1+CH2,       2us  ARR=1,   CCR1=0, CCR2=0
 *            al terminar: CH1 baja, CH1N sube (complementario+deadtime), CH2 sigue
 *   TRAMO 3: CH2+CH1N,     28us  ARR=27,  CCR2=0, CCR1=100
 *            al terminar: CH2 LOW (ultima instruccion de programa)
 *            CH1N sigue activo, timer continua
 *   TRAMO 4: CH1N solo,    72us  ARR=71,  CCR1=100
 *            al terminar: todo apagado (fin hardware)
 *
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
uint8_t Discharge_RunCycle_Unico(void);

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
