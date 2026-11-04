/* stm32f1xx_it.c — USER CODE sections a agregar/modificar
 *
 * INSTRUCCIONES:
 * Copiar cada bloque dentro de los comentarios USER CODE
 * correspondientes en tu stm32f1xx_it.c generado por CubeMX.
 * NO reemplaces el archivo completo, solo agrega dentro de los bloques.
 */

/* ── 1. En la seccion USER CODE BEGIN Includes ─────────────────────── */
#include "discharge.h"

/* ── 2. Variables externas — USER CODE BEGIN EV ───────────────────── */
extern volatile uint8_t btn_pressed;
extern volatile uint32_t sys_tick_ms;

/* ── 3. SysTick_Handler — USER CODE BEGIN SysTick_IRQn 0 ──────────── */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */
  sys_tick_ms++;
  /* USER CODE END SysTick_IRQn 0 */

  /* USER CODE BEGIN SysTick_IRQn 1 */
  /* USER CODE END SysTick_IRQn 1 */
}

/* ── 4. EXTI4_IRQHandler — USER CODE BEGIN LL_EXTI_LINE_4 ─────────── */
/*
 * Reemplazar el cuerpo del USER CODE BEGIN LL_EXTI_LINE_4 con:
 */
void EXTI4_IRQHandler(void)
{
  if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_4) != RESET)
  {
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_4);
    /* USER CODE BEGIN LL_EXTI_LINE_4 */
    btn_pressed = 1;   /* La FSM en main() decide si lo procesa o ignora */
    /* USER CODE END LL_EXTI_LINE_4 */
  }
}

/* ── 5. TIM1_UP_IRQHandler — AGREGAR esta funcion completa ────────── */
/*
 * Agregar antes del USER CODE BEGIN 1 al final del archivo:
 */
void TIM1_UP_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_UP_IRQn 0 */
  Discharge_IRQHandler();
  /* USER CODE END TIM1_UP_IRQn 0 */
}

/* ── 6. En NVIC — agregar en MX_GPIO_Init o al final de Discharge_Init
 *    (ya esta incluido en Discharge_Init, no hace falta duplicarlo) ── */

/*
 * NOTA SOBRE PRIORIDADES:
 * TIM1_UP_IRQn → prioridad 0 (maxima) — definido en Discharge_Init()
 * EXTI4_IRQn   → prioridad 0 — definido en MX_GPIO_Init()
 * SysTick_IRQn → prioridad 15 (minima entre estos) — definido en main()
 *
 * Si hay conflicto entre TIM1_UP y EXTI4 (misma prioridad),
 * cambiar EXTI4 a prioridad 1 en MX_GPIO_Init:
 *   NVIC_SetPriority(EXTI4_IRQn, NVIC_EncodePriority(..., 1, 0));
 */
