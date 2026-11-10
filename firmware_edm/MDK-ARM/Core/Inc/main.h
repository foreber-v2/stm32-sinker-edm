/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   Common defines, pin map, timing constants and FSM states.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_ll_adc.h"
#include "stm32f1xx_ll_dma.h"
#include "stm32f1xx_ll_rcc.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_system.h"
#include "stm32f1xx_ll_exti.h"
#include "stm32f1xx_ll_cortex.h"
#include "stm32f1xx_ll_utils.h"
#include "stm32f1xx_ll_pwr.h"
#include "stm32f1xx_ll_tim.h"
#include "stm32f1xx_ll_usart.h"
#include "stm32f1xx_ll_gpio.h"

#if defined(USE_FULL_ASSERT)
#include "stm32_assert.h"
#endif /* USE_FULL_ASSERT */

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */

/* NVIC priority grouping (used when the LL/HAL headers do not provide them) */
#ifndef NVIC_PRIORITYGROUP_0
#define NVIC_PRIORITYGROUP_0         ((uint32_t)0x00000007) /*!< 0 bit  for pre-emption priority, 4 bits for subpriority */
#define NVIC_PRIORITYGROUP_1         ((uint32_t)0x00000006) /*!< 1 bit  for pre-emption priority, 3 bits for subpriority */
#define NVIC_PRIORITYGROUP_2         ((uint32_t)0x00000005) /*!< 2 bits for pre-emption priority, 2 bits for subpriority */
#define NVIC_PRIORITYGROUP_3         ((uint32_t)0x00000004) /*!< 3 bits for pre-emption priority, 1 bit  for subpriority */
#define NVIC_PRIORITYGROUP_4         ((uint32_t)0x00000003) /*!< 4 bits for pre-emption priority, 0 bit  for subpriority */
#endif

/* USER CODE END Private defines */

/* -- Pin definitions -------------------------------------------------------- */
/* USER CODE BEGIN PD */

/* Status LEDs */
#define LED_GREEN_PIN           LL_GPIO_PIN_4
#define LED_GREEN_PORT          GPIOB

#define LED_RED_PIN             LL_GPIO_PIN_3
#define LED_RED_PORT            GPIOB

#define LED_SAFE_PIN            LL_GPIO_PIN_13
#define LED_SAFE_PORT           GPIOC

/* Power stage */
#define RELAY_MASTER_PIN        LL_GPIO_PIN_5
#define RELAY_MASTER_PORT       GPIOB

#define RELAY_CARGA_PIN         LL_GPIO_PIN_7
#define RELAY_CARGA_PORT        GPIOB

#define MOSFET_BLEEDER_PIN      LL_GPIO_PIN_1
#define MOSFET_BLEEDER_PORT     GPIOB

/* Push buttons (external pull-up, active LOW) */
#define BTN_GO_PIN              LL_GPIO_PIN_4
#define BTN_GO_PORT             GPIOA

#define BTN_A_PIN               LL_GPIO_PIN_15
#define BTN_A_PORT              GPIOA

#define BTN_B_PIN               LL_GPIO_PIN_10
#define BTN_B_PORT              GPIOA

/* -- Timing constants (SysTick = 1 ms) -------------------------------------- */
#define TIME_RELAY_WAIT_MS      8000    /* Capacitor charge time after relays ON    */
#define TIME_RELAY_DEL_WAIT_MS  100     /* Settling time after charge relay release */
#define TIME_LOCKOUT_MS         500     /* Lockout time after a manual stop         */
#define TIME_BLEEDING_WAIT_MS   8000    /* Total bleeding (discharge) time          */
#define TIME_BLINK_IDLE_MS      500     /* Green LED blink period in IDLE           */
#define TIME_BLINK_LOCKOUT_MS   200     /* Red / safe LED blink period              */
#define NUMBER_STEPS_DOWN_MAX   3500     /* Red / safe LED blink period              */

/* -- Stepper direction arguments -------------------------------------------- */
#define BAJA                    0       /* Move down (towards workpiece) */
#define SUBE                    1       /* Move up   (away from workpiece) */

/* -- Debug / test parameters ------------------------------------------------ */
#define TARGET_SPARKS           10000U  /* Only used by the commented-out debug counters in main.c */

/* USER CODE END PD */

/* -- FSM states ------------------------------------------------------------- */
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_IDLE = 0,         /* Green LED blinks, waits for GO button, relays OFF           */
    STATE_RELAY_ON,         /* Relays ON, capacitors charging (TIME_RELAY_WAIT_MS)         */
    STATE_WAIT_XMS,          /* Charge relay released, short settling wait                  */
    STATE_READY,            /* Green LED steady, waits for GO / A / B button               */
    STATE_FORWARD,          /* Stepper moving down (approach), waits until it finishes     */
    STATE_BACKWARD,         /* Stepper moving up (retract), waits until it finishes        */
    STATE_READING,          /* Evaluates sparks / shorts and decides: move or keep sparking*/
    STATE_DISCHARGE_CONT,   /* Pulse train (or single discharge) in progress               */
    STATE_DISCHARGE_ONCE,   /* Single pulse in progress (debug path only)                  */
    STATE_BLEEDING,         /* Bleeder active, safe LED blinks, then back to IDLE          */
    STATE_LOCKOUT           /* Blocked for TIME_LOCKOUT_MS, red LED blinks                 */
} AppState_t;
/* USER CODE END PTD */

/* -- Private function prototypes -------------------------------------------- */
/* USER CODE BEGIN PFP */
void SystemClock_Config(void);
/* NOTE: the static helpers (led_*, relay_*, mosfet_*, all_outputs_safe, get_tick)
 * are declared/defined inside main.c. Static prototypes do not belong in a header
 * because every file that includes main.h would get "declared static but never
 * defined" warnings. */
/* USER CODE END PFP */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */