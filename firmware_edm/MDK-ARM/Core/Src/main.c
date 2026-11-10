/**
 ******************************************************************************
 * @file    main.c
 * @brief   Main application - Blue Pill (STM32F103C8)
 *          Capacitor-discharge machining controller (stepper + PWM discharge).
 ******************************************************************************
 *
 * FINITE STATE MACHINE (summary - full diagram in fsm_diagram.mermaid)
 *
 *   IDLE --GO--> RELAY_ON --8 s--> WAIT_XMS --100 ms--> READY
 *
 *   READY --GO--> READING <---------------------------------------+
 *   READY --A---> LOCKOUT --500 ms--> READY                       |
 *   READY --B---> BLEEDING --8 s--> IDLE                          |
 *                                                                 |
 *   READING --no sparks / no shorts--> FORWARD  --stepper done--> DISCHARGE_CONT
 *   READING --otherwise--------------> BACKWARD --stepper done--> DISCHARGE_CONT
 *   READING --sparks > shorts--------> DISCHARGE_CONT (pulse train)
 *   READING --down_steps_done > 3500-> READY
 *
 *   DISCHARGE_CONT --done, B pressed--> LOCKOUT
 *   DISCHARGE_CONT --done-------------> READING
 *
 * PIN MAP
 *   PB4   LED green        (driven LOW = ON)
 *   PB3   LED red          (driven LOW = ON)
 *   PC13  LED safe         (driven LOW = ON)
 *   PB5   Master relay     (driven HIGH = ON)
 *   PB7   Charge relay     (driven LOW  = ON)
 *   PB1   Bleeder MOSFET   (driven LOW  = ON)
 *   PA4   Button GO        (external pull-up, active LOW, EXTI4 falling edge)
 *   PA15  Button A         (external pull-up, active LOW)
 *   PA10  Button B         (external pull-up, active LOW)
 *   PA8   TIM1_CH1         (discharge PWM)
 *   PA9   TIM1_CH2
 *   PB13  TIM1_CH1N        (complementary PWM)
 *
 * NOTE: Lines marked "DEBUG" are commented-out code kept from bring-up and
 *       testing sessions. They are NOT needed for normal operation. They can be
 *       deleted safely, or re-enabled to run additional tests.
 ******************************************************************************
 */

/* USER CODE BEGIN Header */
/* USER CODE END Header */

/* -- Includes --------------------------------------------------------------- */
/* USER CODE BEGIN Includes */
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "discharge.h"
#include "stepper.h"
#include "global_vars.h"
/* USER CODE END Includes */

/* -- Private variables ------------------------------------------------------ */
/* USER CODE BEGIN PV */

/* FSM state and approach-step counter */
static volatile AppState_t app_state       = STATE_IDLE;
static volatile uint32_t   down_steps_done = 0;   /* Consecutive downward steps without finding sparks */

/* DEBUG: counters used only by the commented-out test code below.
 * Not needed for normal operation. */
volatile uint8_t  tren               = 0;   /* DEBUG: flag, a pulse train was launched   */
volatile uint32_t sparks_effective   = 0;   /* DEBUG: accumulated effective sparks       */
volatile uint32_t sparks_ineffective = 0;   /* DEBUG: accumulated ineffective sparks     */
volatile uint32_t test_start_ms      = 0;   /* DEBUG: test start timestamp               */
volatile uint32_t test_elapsed_ms    = 0;   /* DEBUG: test elapsed time                  */

/* USER CODE END PV */

/* -- Private helpers (static) ----------------------------------------------- */
/* USER CODE BEGIN 0 */

/* Time base */
static uint32_t get_tick(void)              { return sys_tick_ms; }

/* Status LEDs (active LOW) */
static void led_green_on(void)              { LL_GPIO_ResetOutputPin(LED_GREEN_PORT, LED_GREEN_PIN); }
static void led_green_off(void)             { LL_GPIO_SetOutputPin(LED_GREEN_PORT, LED_GREEN_PIN); }
static void led_red_on(void)                { LL_GPIO_ResetOutputPin(LED_RED_PORT, LED_RED_PIN); }
static void led_red_off(void)               { LL_GPIO_SetOutputPin(LED_RED_PORT, LED_RED_PIN); }
static void led_safe_on(void)               { LL_GPIO_ResetOutputPin(LED_SAFE_PORT, LED_SAFE_PIN); }
static void led_safe_off(void)              { LL_GPIO_SetOutputPin(LED_SAFE_PORT, LED_SAFE_PIN); }

/* Bleeder MOSFET (active LOW) */
static void mosfet_bleeder_on(void)         { LL_GPIO_ResetOutputPin(MOSFET_BLEEDER_PORT, MOSFET_BLEEDER_PIN); }
static void mosfet_bleeder_off(void)        { LL_GPIO_SetOutputPin(MOSFET_BLEEDER_PORT, MOSFET_BLEEDER_PIN); }

/* Master relay (active HIGH) */
static void relay_master_on(void)           { LL_GPIO_SetOutputPin(RELAY_MASTER_PORT, RELAY_MASTER_PIN); }
static void relay_master_off(void)          { LL_GPIO_ResetOutputPin(RELAY_MASTER_PORT, RELAY_MASTER_PIN); }

/* Charge relay (active LOW) */
static void relay_charge_on(void)           { LL_GPIO_ResetOutputPin(RELAY_CARGA_PORT, RELAY_CARGA_PIN); }
static void relay_carga_off(void)           { LL_GPIO_SetOutputPin(RELAY_CARGA_PORT, RELAY_CARGA_PIN); }

/* Puts every output in its safe (power-off) state */
static void all_outputs_safe(void)
{
    led_green_off();
    led_red_off();
    relay_master_off();
    relay_charge_on();
    mosfet_bleeder_on();
    /* TIM1 is already left in a safe state by Discharge_Init() / tim1_stop() */
}

/* USER CODE END 0 */

/* -- main ------------------------------------------------------------------- */
int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    /* Core / system configuration */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));

    LL_GPIO_AF_Remap_SWJ_NOJTAG();   /* Frees PB3, PB4 (and PA15) from JTAG */

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();            /* HSE 8 MHz + PLL -> 72 MHz */

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    /* Peripheral initialization */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();
    MX_TIM4_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */
    all_outputs_safe();
    Discharge_Init();                /* Configures TIM1 so the FSM is driven by IRQ */
    Stepper_Init();
    /* USER CODE END 2 */

    /* -- FSM local variables ------------------------------------------------ */
    /* USER CODE BEGIN WHILE */
    uint32_t state_entry_tick = get_tick();   /* ms at which the current state was entered */
    uint32_t last_blink_tick  = get_tick();   /* ms of the last LED toggle                 */
    uint8_t  blink_state      = 0;

    app_state = STATE_IDLE;

    while (1)
    {
        uint32_t now = sys_tick_ms;

        switch (app_state)
        {
            /* ------------------------------------------------------------------
             * IDLE: green LED blinks (non-blocking), waits for GO button.
             * Next: RELAY_ON
             * ------------------------------------------------------------------ */
            case STATE_IDLE:
                /* Non-blocking blink */
                if ((now - last_blink_tick) >= TIME_BLINK_IDLE_MS)
                {
                    last_blink_tick = now;
                    blink_state = !blink_state;
                    if (blink_state) led_green_on(); else led_green_off();
                }

                if (btn_go_pressed)
                {
                    btn_go_pressed = 0;

                    /* Transition to RELAY_ON: start charging the capacitors */
                    led_green_off();
                    led_red_on();
                    mosfet_bleeder_off();
                    relay_master_on();
                    relay_charge_on();

                    app_state = STATE_RELAY_ON;
                    state_entry_tick = now;
                }
                break;

            /* ------------------------------------------------------------------
             * RELAY_ON: relays ON, red LED steady, waits TIME_RELAY_WAIT_MS
             * for the capacitors to charge.
             * Next: WAIT_XMS
             * ------------------------------------------------------------------ */
            case STATE_RELAY_ON:
                if ((now - state_entry_tick) >= TIME_RELAY_WAIT_MS)
                {
                    led_red_on();
                    app_state = STATE_WAIT_XMS;
                    state_entry_tick = now;
                }
                break;

            /* ------------------------------------------------------------------
             * WAIT_XMS: charge relay released, waits TIME_RELAY_DEL_WAIT_MS
             * (red LED steady) until it is fully off. Buttons are ignored.
             * Next: READY
             * ------------------------------------------------------------------ */
            case STATE_WAIT_XMS:
                relay_carga_off();

                if ((now - state_entry_tick) >= TIME_RELAY_DEL_WAIT_MS)
                {
                    led_red_off();
                    led_green_on();
                    app_state = STATE_READY;
                    state_entry_tick = now;
                }

                /* Ignore any button press during the wait */
                btn_go_pressed = 0;
                btn_a_pressed  = 0;
                btn_b_pressed  = 0;
                break;

            /* ------------------------------------------------------------------
             * READY: green LED steady, machine armed.
             *   GO -> READING   (start automatic approach / machining)
             *   A  -> LOCKOUT   (fast stepper move up)
             *   B  -> BLEEDING  (shut down power stage and discharge capacitors)
             * ------------------------------------------------------------------ */
            case STATE_READY:
                sparks = 0;
                shorts = 0;

                /* GO: start machining */
                if (btn_go_pressed)
                {
                    btn_go_pressed = 0;
                    btn_a_pressed  = 0;
                    btn_b_pressed  = 0;

                    led_green_off();

                    /* DEBUG alternatives (not needed for normal operation):
                     *   Discharge_RunCycle_Once();        // final single-discharge cycle
                     *   Discharge_RunCycle_Train();        // debug only: approach test without electrolysis
                     *   Stepper_Mov_Slow(BAJA);
                     *   app_state = STATE_DISCHARGE_ONCE;
                     */
                    app_state = STATE_READING;

                    /* DEBUG: test counters (re-enable together with the DEBUG block in STATE_READING)
                     *   sparks_effective   = 0;
                     *   sparks_ineffective = 0;
                     *   test_start_ms      = sys_tick_ms;
                     *   test_elapsed_ms    = 0;
                     */
                }

                /* A: fast move up, then lockout */
                if (btn_a_pressed)
                {
                    btn_go_pressed = 0;
                    btn_a_pressed  = 0;
                    btn_b_pressed  = 0;

                    led_green_on();
                    led_red_on();

                    /* DEBUG: Discharge_RunCycle_Train(); */
                    Stepper_Mov_Fast(SUBE);
                    app_state = STATE_LOCKOUT;
										state_entry_tick = now;
                }

                /* B: transition to BLEEDING (relays off, capacitors discharge) */
                if (btn_b_pressed)
                {
                    btn_a_pressed  = 0;
                    btn_b_pressed  = 0;
                    btn_go_pressed = 0;

                    led_green_off();
                    relay_master_off();
                    relay_charge_on();
                    /* mosfet_bleeder_on() is called later, inside STATE_BLEEDING */

                    app_state = STATE_BLEEDING;
                    state_entry_tick = now;
                }
                break;

            /* ------------------------------------------------------------------
             * FORWARD: stepper moves in the commanded direction (down).
             * Waits until the move ends, then fires a single discharge.
             * Next: DISCHARGE_CONT
             * ------------------------------------------------------------------ */
            case STATE_FORWARD:
                if (!Stepper_IsBusy())
                {
                    Discharge_RunCycle_Once();
                    app_state = STATE_DISCHARGE_CONT;
                }
                break;

            /* ------------------------------------------------------------------
             * BACKWARD: stepper moves in the commanded direction (up).
             * Waits until the move ends, then fires a single discharge.
             * Next: DISCHARGE_CONT
             * ------------------------------------------------------------------ */
            case STATE_BACKWARD:
                if (!Stepper_IsBusy())
                {
                    Discharge_RunCycle_Once();
                    app_state = STATE_DISCHARGE_CONT;
                }
                break;

            /* ------------------------------------------------------------------
             * READING: decision state. Once the discharge driver is idle,
             * analyses the sparks / shorts counters and decides:
             *   1. Too many steps down without sparks -> give up, back to READY
             *   2. Enough good sparks                 -> keep sparking (pulse train)
             *   3. No sparks and no shorts            -> step down (approach)
             *   4. Otherwise (shorts dominate)        -> step up (retract)
             * ------------------------------------------------------------------ */
            case STATE_READING:
                if (!Discharge_IsBusy())
                {
                    led_green_on();
                    led_red_on();
                    btn_go_pressed = 0;
                    btn_a_pressed  = 0;

                    if (down_steps_done > NUMBER_STEPS_DOWN_MAX)   
                    {
                        /* 1. Gap not found: abort and return to READY */
                        sparks = 0;
                        shorts = 0;
                        down_steps_done = 0;
                        app_state = STATE_READY;
                    }
                    else if ((sparks > 1) && (sparks > shorts) && (shorts >= 0))
                    {
                        /* 2. Good machining condition: run a pulse train */
                        down_steps_done = 0;
                        sparks = 0;
                        shorts = 0;

                        /* DEBUG: effective-spark counting and target check
                         *   sparks_effective++;
                         *   if (sparks_effective >= TARGET_SPARKS)
                         *   {
                         *       test_elapsed_ms = sys_tick_ms - test_start_ms;
                         *       app_state = STATE_READY;
                         *   }
                         *   else
                         *   {
                         *       tren = 1;
                         *       Discharge_RunCycle_Train();
                         *       app_state = STATE_DISCHARGE_CONT;
                         *   }
                         */

                        Discharge_RunCycle_Train();
                        app_state = STATE_DISCHARGE_CONT;
                    }
                    else if ((sparks < 1) && (shorts < 1))
                    {
                        /* 3. Nothing detected: approach one step down */
                        down_steps_done++;
                        sparks = 0;
                        shorts = 0;
                        Stepper_Mov_Slow(BAJA);
                        app_state = STATE_FORWARD;
                    }
                    else
                    {
                        /* 4. Shorts dominate: retract (mid-size step up) */
                        down_steps_done = 0;
                        sparks = 0;
                        shorts = 0;
                        Stepper_Mov_Mid(SUBE);
                        app_state = STATE_BACKWARD;
                    }
                }
                break;

            /* ------------------------------------------------------------------
             * DISCHARGE_CONT: discharge cycle (single or pulse train) running.
             * When the driver finishes:
             *   B pressed -> LOCKOUT (operator stop)
             *   otherwise -> READING (evaluate result)
             * ------------------------------------------------------------------ */
            case STATE_DISCHARGE_CONT:
                if (!Discharge_IsBusy())
                {
                    led_green_off();
                    led_red_off();

                    if (btn_b_pressed)
                    {
                        btn_go_pressed = 0;
                        btn_a_pressed  = 0;
                        btn_b_pressed  = 0;

                        led_green_off();
                        app_state = STATE_LOCKOUT;

                        /* DEBUG: test statistics reset
                         *   test_elapsed_ms    = sys_tick_ms - test_start_ms;
                         *   sparks_effective   = 0;
                         *   sparks_ineffective = 0;
                         *   test_elapsed_ms    = 0;
                         *   tren               = 0;
                         */

                        state_entry_tick = now;
                        last_blink_tick  = now;
                        blink_state = 0;
                    }
                    else
                    {
                        btn_go_pressed = 0;
                        btn_a_pressed  = 0;

                        /* DEBUG: effective / ineffective spark accounting
                         *   Discharge_RunCycle_Train();
                         *   sparks_ineffective++;
                         *   if (tren && (sparks > 4) && ())      // condition was never completed
                         *   {
                         *       sparks_effective   = sparks_effective   + sparks - 4u;
                         *       sparks_ineffective = sparks_ineffective + 4u;
                         *   }
                         *   else
                         *   {
                         *       sparks_effective   = sparks_effective   + sparks - 1u;
                         *       sparks_ineffective = sparks_ineffective + 1u;
                         *   }
                         *   tren = 0;
                         */

                        app_state = STATE_READING;
                    }
                }
                break;

            /* ------------------------------------------------------------------
             * DISCHARGE_ONCE: single pulse running (reachable only through the
             * DEBUG path in STATE_READY). Returns to READING when finished.
             * ------------------------------------------------------------------ */
            case STATE_DISCHARGE_ONCE:
                if (!Discharge_IsBusy())
                {
                    btn_go_pressed = 0;
                    btn_a_pressed  = 0;
                    btn_b_pressed  = 0;

                    app_state = STATE_READING;
                    state_entry_tick = now;
                    last_blink_tick  = now;
                    blink_state = 0;
                }
                break;

            /* ------------------------------------------------------------------
             * BLEEDING: relays are off. After TIME_RELAY_DEL_WAIT_MS the bleeder
             * MOSFET turns on; safe LED blinks until TIME_BLEEDING_WAIT_MS.
             * Buttons are ignored.
             * Next: IDLE
             * ------------------------------------------------------------------ */
            case STATE_BLEEDING:
                if ((now - state_entry_tick) >= TIME_RELAY_DEL_WAIT_MS)
                {
                    mosfet_bleeder_on();
                }

                if ((now - last_blink_tick) >= TIME_BLINK_LOCKOUT_MS)
                {
                    last_blink_tick = now;
                    blink_state = !blink_state;
                    if (blink_state) led_safe_on(); else led_safe_off();
                }

                if ((now - state_entry_tick) >= TIME_BLEEDING_WAIT_MS)
                {
                    led_safe_off();
                    led_green_on();
                    app_state = STATE_IDLE;
                    state_entry_tick = now;
                }

                /* Ignore any button press during bleeding */
                btn_go_pressed = 0;
                btn_a_pressed  = 0;
                btn_b_pressed  = 0;
                break;

            /* ------------------------------------------------------------------
             * LOCKOUT: blocked for TIME_LOCKOUT_MS, red LED blinks fast.
             * I_Limit_Max is cleared and buttons are ignored.
             * Next: READY
             * ------------------------------------------------------------------ */
            case STATE_LOCKOUT:
                /* Fast red LED blink */
                if ((now - last_blink_tick) >= TIME_BLINK_LOCKOUT_MS)
                {
                    last_blink_tick = now;
                    blink_state = !blink_state;
                    if (blink_state) led_red_on(); else led_red_off();
                }

                if ((now - state_entry_tick) >= TIME_LOCKOUT_MS)
                {
                    led_red_off();
                    led_green_on();
                    app_state = STATE_READY;
                    state_entry_tick = now;
                }

                I_Limit_Max    = 0;   /* Restart the over-current counter */
                btn_go_pressed = 0;   /* Ignore any button press during lockout */
                btn_a_pressed  = 0;
                btn_b_pressed  = 0;
                break;

            /* ------------------------------------------------------------------
             * Fallback: unknown state -> safe outputs and restart from IDLE.
             * ------------------------------------------------------------------ */
            default:
                all_outputs_safe();
                app_state = STATE_IDLE;
                state_entry_tick = now;
                break;
        }
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/* -- SystemClock_Config: HSE 8 MHz + PLL x9 -> 72 MHz ------------------------ */
void SystemClock_Config(void)
{
    /* Flash latency for 72 MHz */
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2) {}

    /* HSE on */
    LL_RCC_HSE_Enable();
    while (LL_RCC_HSE_IsReady() != 1) {}

    /* PLL: source HSE, multiplier x9 -> 8 * 9 = 72 MHz */
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE_DIV_1, LL_RCC_PLL_MUL_9);
    LL_RCC_PLL_Enable();
    while (LL_RCC_PLL_IsReady() != 1) {}

    /* Bus prescalers */
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);   /* AHB  = 72 MHz */
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);    /* APB1 = 36 MHz */
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);    /* APB2 = 72 MHz */

    /* Switch system clock source to PLL */
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {}

    /* SysTick at 1 ms */
    LL_Init1msTick(72000000);
    LL_SetSystemCoreClock(72000000);
    LL_SYSTICK_EnableIT();

    /* ADC clock */
    LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSRC_PCLK2_DIV_6);   /* 72 / 6 = 12 MHz */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/* -- Error handler: disable IRQs, force safe outputs, halt ------------------- */
void Error_Handler(void)
{
    __disable_irq();
    all_outputs_safe();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif