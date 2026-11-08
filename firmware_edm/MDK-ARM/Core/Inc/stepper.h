
#ifndef STEPPER_H 
#define STEPPER_H

#include "stm32f1xx.h"
#include <stdint.h>

#define NUM_APROX_STEPS 			 1
#define NUM_STEPS_MID   			 50
#define NUM_QUICK_STEPS			 35000
#define TIME_BETWEEN_STEPS		 300u

/* -- API publica ------------------------------------------------------- */

/**
 * @brief  Inicializa y ajusta TIM2 para el ciclo de pasos.
 *         Llamar UNA VEZ despues de MX_TIM2_Init().
 *         Deja el timer listo pero sin generar pasos.
 */
void Stepper_Init(void);

/**
 * @brief  Inicia secuencia de pasos para iniciar el tim2.
 *         Limpia flags y eso
 */
static void Stepper_timer_run(uint32_t consigna);

/**
 * @brief  Inicia un ciclo para moverse poco a poco.
 *         No hace nada si ya hay un ciclo en curso.
 * @retval 1 si el ciclo fue disparado, 0 si estaba ocupado
 */
uint8_t Stepper_Aprox_Inicial(uint8_t direction);

/**
 * @brief  Inicia un ciclo para moverse a poco.
 *         No hace nada si ya hay un ciclo en curso.
 * @retval 1 si el ciclo fue disparado, 0 si estaba ocupado
 */
 uint8_t Stepper_Mov_Mid(uint8_t direction);

/**
 * @brief  Inicia un ciclo para moverse a velocidad un poco mayor que baja.
 *         No hace nada si ya hay un ciclo en curso.
 * @retval 1 si el ciclo fue disparado, 0 si estaba ocupado
 */
uint8_t Stepper_Mov_Rapido(uint8_t direction);

/**
 * @brief  Para moverse rapido, detiene el timer
 *         Reinicia valores pertinentes.
 */
void Stepper_Stop(void);

/**
 * @brief  Consulta si hay un ciclo en curso.
 * @retval 1 = ocupado, 0 = libre
 */
uint8_t Stepper_IsBusy(void);

/**
 * @brief  Llamar desde TIM2_IRQHandler.
 *         Avanza la FSM interna del ciclo de pasos.
 */
void Stepper_IRQHandler(void);

#endif /* STEPPER_H */
