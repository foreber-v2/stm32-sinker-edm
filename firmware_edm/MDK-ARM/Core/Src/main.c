/* main.c — Blue Pill STM32F103C8
 * FSM principal: IDLE ? RELE_ON ? WAIT_5S ? READY ? DISCHARGE ? LOCKOUT ? READY
 *
 * Pines:
 *   PB4  LED verde  (pull-up externo, activo HIGH)
 *   PB3  LED rojo   (pull-up externo, activo HIGH)
 *   PB5  Rele       (activo HIGH)
 *   PA4  Pulsador   (pull-up externo, activo LOW — EXTI4 falling edge)
 *   PA8  TIM1_CH1   (PWM descarga)
 *   PA9  TIM1_CH2
 *   PB13 TIM1_CH1N  (PWM complementario)
 */

/* USER CODE BEGIN Header */
/* USER CODE END Header */

#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "discharge.h"
#include "stepper.h"
#include "global_vars.h" 
/* USER CODE END Includes */

/* -- Definiciones de pines --------------------------------------------- */
/* USER CODE BEGIN PD */
#define LED_GREEN_PIN       LL_GPIO_PIN_4
#define LED_GREEN_PORT      GPIOB

#define LED_RED_PIN         LL_GPIO_PIN_3
#define LED_RED_PORT        GPIOB

#define RELAY_MASTER_PIN    LL_GPIO_PIN_5
#define RELAY_MASTER_PORT   GPIOB

#define RELAY_CARGA_PIN     LL_GPIO_PIN_7
#define RELAY_CARGA_PORT    GPIOB

#define MOSFET_BLEEDER_PIN  LL_GPIO_PIN_1
#define MOSFET_BLEEDER_PORT GPIOB

#define BTN_GO_PIN          LL_GPIO_PIN_4
#define BTN_GO_PORT         GPIOA

#define BTN_A_PIN           LL_GPIO_PIN_15
#define BTN_A_PORT          GPIOA

#define BTN_B_PIN           LL_GPIO_PIN_10
#define BTN_B_PORT          GPIOA

#define LED_SAFE_PIN        LL_GPIO_PIN_13	
#define LED_SAFE_PORT       GPIOC

/* Tiempos en ms (SysTick a 1ms) */
#define TIME_RELAY_WAIT_MS      8000
#define TIME_RELAY_DEL_WAIT_MS  100
#define TIME_LOCKOUT_MS         500
#define TIME_BLEEDING_WAIT_MS   8000
#define TIME_BLINK_IDLE_MS      500	  /* parpadeo LED verde en IDLE */
#define TIME_BLINK_LOCKOUT_MS   200   /* parpadeo LED rojo en LOCKOUT */
#define BAJA										0
#define SUBE										1

volatile uint8_t tren = 0;
volatile uint32_t sparks_effective = 0;
volatile uint32_t sparks_ineffective = 0;

volatile uint32_t test_start_ms = 0;
volatile uint32_t test_elapsed_ms = 0;

#define TARGET_SPARKS 10000U

/* USER CODE END PD */

/* -- Estados de la FSM ------------------------------------------------- */
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_IDLE = 0,     		/* LED verde parpadea, espera boton reles apagados */
    STATE_RELAY_ON,     		/* Reles activados, transicion inmediata */
    STATE_WAIT_5S,      		/* LED rojo fijo, 5s de espera */
    STATE_READY,        		/* LED verde fijo, espera boton para descarga */
		STATE_AVANCE,						/* Stepper inicia un ciclo de pasos adelante y espera */
		STATE_RETROCESO,				/* Stepper inicia un ciclo de pasos atras y espera */
		STATE_LECTURA,					/* Se leen los datos de shorts y sparks, se mueve o da descargas acorde */
    STATE_DISCHARGE_CONT, 	/* Ejecutando ciclo de pulsos */
		STATE_DISCHARGE_UNICO,  /* Ejecutando pulso unico */
		STATE_BLEEDING,					/* Se bloquea y despliega led safe, mientras bleeding */
    STATE_LOCKOUT       		/* 5s bloqueado post-descarga, LED rojo parpadea */
} AppState_t;
/* USER CODE END PTD */

/* -- Variables globales ------------------------------------------------ */
/* USER CODE BEGIN PV */
static volatile AppState_t app_state = STATE_IDLE;
static volatile uint32_t down_steps_done = 0;

/* USER CODE END PV */

/* -- Prototipos privados ----------------------------------------------- */
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static uint32_t get_tick(void);
static void led_green_on(void);
static void led_green_off(void);
static void led_red_on(void);
static void led_red_off(void);
static void led_safe_on(void);
static void led_safe_off(void);
static void mosfet_bleeder_on(void);
static void mosfet_bleeder_off(void);
static void relay_master_on(void);
static void relay_master_off(void);
static void relay_charge_on(void);
static void relay_carga_off(void);
static void all_outputs_safe(void);
/* USER CODE END PFP */

/* -- Inline helpers ---------------------------------------------------- */
/* USER CODE BEGIN 0 */
static uint32_t get_tick(void)        { return sys_tick_ms; }
static void led_green_on(void)        { LL_GPIO_ResetOutputPin(LED_GREEN_PORT, LED_GREEN_PIN); }
static void led_green_off(void)       { LL_GPIO_SetOutputPin(LED_GREEN_PORT, LED_GREEN_PIN); }
static void led_red_on(void)          { LL_GPIO_ResetOutputPin(LED_RED_PORT, LED_RED_PIN); }
static void led_red_off(void)         { LL_GPIO_SetOutputPin(LED_RED_PORT, LED_RED_PIN); }
static void led_safe_on(void)         { LL_GPIO_ResetOutputPin(LED_SAFE_PORT, LED_SAFE_PIN); }
static void led_safe_off(void)        { LL_GPIO_SetOutputPin(LED_SAFE_PORT, LED_SAFE_PIN); }
static void mosfet_bleeder_on(void)		{ LL_GPIO_ResetOutputPin(MOSFET_BLEEDER_PORT, MOSFET_BLEEDER_PIN); }
static void mosfet_bleeder_off(void)	{ LL_GPIO_SetOutputPin(MOSFET_BLEEDER_PORT, MOSFET_BLEEDER_PIN); }
static void relay_master_on(void)     { LL_GPIO_SetOutputPin(RELAY_MASTER_PORT, RELAY_MASTER_PIN); }
static void relay_master_off(void)    { LL_GPIO_ResetOutputPin(RELAY_MASTER_PORT, RELAY_MASTER_PIN); }
static void relay_charge_on(void)      { LL_GPIO_ResetOutputPin(RELAY_CARGA_PORT, RELAY_CARGA_PIN); }
static void relay_carga_off(void)     { LL_GPIO_SetOutputPin(RELAY_CARGA_PORT, RELAY_CARGA_PIN); }

static void all_outputs_safe(void)
{
    led_green_off();
    led_red_off();
    relay_master_off();
		relay_charge_on();
		mosfet_bleeder_on();
    /* TIM1 ya queda seguro por Discharge_Init() / tim1_stop() */
}
/* USER CODE END 0 */

/* -- main -------------------------------------------------------------- */
int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));


    LL_GPIO_AF_Remap_SWJ_NOJTAG();   /* Libera PB3, PB4 */

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();            /* HSE 8MHz + PLL ? 72MHz */

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_TIM1_Init();
		MX_TIM2_Init();
    MX_TIM4_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */
    all_outputs_safe();
    Discharge_Init();                /* Ajusta TIM1 para FSM por IRQ */
		Stepper_Init();   
    /* USER CODE END 2 */

    /* -- Variables de la FSM ------------------------------------------- */
    /* USER CODE BEGIN WHILE */
    uint32_t state_entry_tick = get_tick();   /* ms en que entramos al estado */
    uint32_t last_blink_tick  = get_tick();   /* ms del ultimo toggle de LED */
    uint8_t  blink_state      = 0;

    app_state = STATE_IDLE;

    while (1)
    {
        uint32_t now = sys_tick_ms; 

        switch (app_state)
        {
            /* -- IDLE: LED verde parpadea, espera boton ----------------- */
            case STATE_IDLE:
                /* Parpadeo no bloqueante */
                if ((now - last_blink_tick) >= TIME_BLINK_IDLE_MS)
                {
                    last_blink_tick = now;
                    blink_state = !blink_state;
                    if (blink_state) led_green_on(); else led_green_off();
                }
								
                if (btn_go_pressed)
                {
                    btn_go_pressed = 0;
                    /* Transicion a RELAY_ON */
                    led_green_off();
										led_red_on();
										mosfet_bleeder_off();
                    relay_master_on();
										relay_charge_on();										
                    app_state = STATE_RELAY_ON;
                    state_entry_tick = now;										
                }
                break;

            /* -- RELAY_ON: rele encendido, espera 5s carga caps con LED rojo ------- */
            case STATE_RELAY_ON:								
								if ((now - state_entry_tick) >= TIME_RELAY_WAIT_MS)
                {
										led_red_on();
										app_state = STATE_WAIT_5S;	
										state_entry_tick = now;
                }                
                break;

            /* -- WAIT_5S: espera 100ms con LED rojo fijo espera apagado total rele ------------------ */
            case STATE_WAIT_5S:
								relay_carga_off();
                if ((now - state_entry_tick) >= TIME_RELAY_DEL_WAIT_MS)
                {
                    led_red_off();
                    led_green_on();
                    app_state = STATE_READY;
                    state_entry_tick = now;
                }
								btn_go_pressed = 0;   /* Ignorar cualquier pulsacion durante espera */
								btn_a_pressed = 0;		/* Ignorar cualquier pulsacion durante espera */
								btn_b_pressed = 0;		/* Ignorar cualquier pulsacion durante espera */
                break;
								
            /* -- READY: LED verde fijo, espera boton para descarga ------- */
            case STATE_READY:
								sparks = 0;
								shorts = 0;
                if (btn_go_pressed)
                {
                    btn_go_pressed = 0;
										btn_a_pressed = 0;		
										btn_b_pressed = 0;
                    led_green_off();
                    /* Disparar ciclo de descarga */
										//Discharge_RunCycle_Unico(); // final
                    //Discharge_RunCycle_Train();	// habilitar solo para debugging aproximacion no electrolisis
										//Stepper_Aprox_Inicial(BAJA);
                    //app_state = STATE_DISCHARGE_UNICO;
										app_state = STATE_LECTURA;
//										sparks_effective = 0;
//										sparks_ineffective = 0;

//										test_start_ms = sys_tick_ms;
//										test_elapsed_ms = 0;
                }
								
								if (btn_a_pressed)
                {
                    btn_go_pressed = 0;
										btn_a_pressed = 0;		
										btn_b_pressed = 0;
										led_green_on();
										led_red_on();                 
                    /* Disparar ciclo de descarga */
                    //Discharge_RunCycle_Train();
										Stepper_Mov_Rapido(SUBE);
                    app_state = STATE_LOCKOUT;
                }
								
								if (btn_b_pressed)
                {
                    btn_a_pressed = 0;
										btn_b_pressed = 0;
										btn_go_pressed = 0;
                    /* Transicion a RELAYS_OFF */
										led_green_off();
                    relay_master_off();
										relay_charge_on();
										//mosfet_bleeder_on();
                    app_state = STATE_BLEEDING;
                    state_entry_tick = now;
                }
                break;
								
						/* -- AVANCE: avanza en el sentido indicado por la funcion espera a que termine avance */
						case STATE_AVANCE:
								if(!Stepper_IsBusy())
								{
									Discharge_RunCycle_Unico();
									app_state = STATE_DISCHARGE_CONT;
								}
								break;
								
						/* -- AVANCE: avanza en el sentido indicado por la funcion espera a que termine avance */
						case STATE_RETROCESO:
								if(!Stepper_IsBusy())
								{
									Discharge_RunCycle_Unico();
									app_state = STATE_DISCHARGE_CONT;
								}
								break;				

						/* -- LECTURA: Cuenta y analiza si es conveniente moverse o quedarse chispeando */
						case STATE_LECTURA:
								if(!Discharge_IsBusy())
								{
									led_green_on();
									led_red_on();
									btn_go_pressed = 0;
									btn_a_pressed = 0;
									if (down_steps_done > 3500) //corrgeir valor de comparacion a variable
									{
										sparks = 0;
										shorts = 0;
										down_steps_done = 0;
										app_state = STATE_READY;
									}
									else if ((sparks > 1) && (sparks > shorts ) && (shorts >= 0))
									{
										down_steps_done = 0;
										sparks = 0;
										shorts = 0;
										//sparks_effective++;

//										if (sparks_effective >= TARGET_SPARKS)
//											{
//													test_elapsed_ms = sys_tick_ms - test_start_ms;
//													app_state = STATE_READY;
//											}
//										else 
//											{
//												tren = 1;
//												Discharge_RunCycle_Train();
//												app_state = STATE_DISCHARGE_CONT;
//											}
											
											Discharge_RunCycle_Train();
											app_state = STATE_DISCHARGE_CONT;
									}
									else if ((sparks < 1) && (shorts < 1))
									{
										down_steps_done++;
										sparks = 0;
										shorts = 0;
										Stepper_Aprox_Inicial(BAJA);
										app_state = STATE_AVANCE;
									}
									else
									{
										down_steps_done = 0;
										sparks = 0;
										shorts = 0;
										Stepper_Mov_Mid(SUBE);
										app_state = STATE_RETROCESO;
									}
								}
								break;

            /* -- DISCHARGE: ciclo de pulsos en curso ------------------- */
            case STATE_DISCHARGE_CONT:
                /* Esperar a que el driver de descarga termine */
                if (!Discharge_IsBusy())
                {
									led_green_off();
									led_red_off();
									if (btn_b_pressed) 
									{
										btn_go_pressed = 0;
										btn_a_pressed = 0;
										btn_b_pressed = 0;
										led_green_off();
										app_state = STATE_LOCKOUT;
										//test_elapsed_ms = sys_tick_ms - test_start_ms;
										//sparks_effective = 0;
										//sparks_ineffective = 0;
										//test_elapsed_ms = 0;
                    state_entry_tick = now;
                    last_blink_tick  = now;
                    blink_state = 0;
										//tren = 0;
									}
                  else
									{
										btn_go_pressed = 0;
										btn_a_pressed = 0;		
                    /* Disparar ciclo de descarga */
                    //Discharge_RunCycle_Train();
										//sparks_ineffective++;
//										if(tren && (sparks>4) && ()
//										{
//											sparks_effective = sparks_effective + sparks - 4u;
//											sparks_ineffective = sparks_ineffective + 4u;
//										}
//										else
//										{
//											sparks_effective = sparks_effective + sparks - 1u;
//											sparks_ineffective = sparks_ineffective + 1u;
//										}
//										tren = 0;
                    app_state = STATE_LECTURA;
									}
                }
                break;
								
							case STATE_DISCHARGE_UNICO:
								
                /* Esperar a que el driver de descarga termine */
                if (!Discharge_IsBusy())
                {
									btn_go_pressed = 0;
									btn_a_pressed = 0;
									btn_b_pressed = 0;
									app_state = STATE_LECTURA;
                  state_entry_tick = now;
                  last_blink_tick  = now;
                  blink_state = 0;
								}
                break;

						/* -- BLEEDING: LED safe parpadea, pasa a IDLE luego ------- */		
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
								btn_go_pressed = 0;   /* Ignorar cualquier pulsacion durante espera */
								btn_a_pressed = 0;		/* Ignorar cualquier pulsacion durante espera */
								btn_b_pressed = 0;		/* Ignorar cualquier pulsacion durante espera */
                break;
								
            /* -- LOCKOUT: 5s bloqueado, LED rojo parpadea --------------- */
            case STATE_LOCKOUT:
                /* Parpadeo rapido LED rojo */
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
								I_Limit_Max = 0;			/* Reinicia el contador al encontrar un corrientazo mas */
								btn_go_pressed = 0;   /* Ignorar cualquier pulsacion durante espera */
								btn_a_pressed = 0;		/* Ignorar cualquier pulsacion durante espera */
								btn_b_pressed = 0;		/* Ignorar cualquier pulsacion durante espera */
                break;

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

/* -- SystemClock_Config — HSE 8MHz + PLL ? 72MHz ---------------------- */
void SystemClock_Config(void)
{
    /* Flash latency para 72MHz */
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2) {}

    /* HSE on */
    LL_RCC_HSE_Enable();
    while (LL_RCC_HSE_IsReady() != 1) {}

    /* PLL: fuente HSE, multiplicador x9 ? 8*9 = 72MHz */
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE_DIV_1, LL_RCC_PLL_MUL_9);
    LL_RCC_PLL_Enable();
    while (LL_RCC_PLL_IsReady() != 1) {}

    /* Bus prescalers */
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);   /* AHB  = 72MHz */
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);    /* APB1 = 36MHz */
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);    /* APB2 = 72MHz */

    /* Cambiar fuente a PLL */
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {}

    /* SysTick a 1ms */
    LL_Init1msTick(72000000);
    LL_SetSystemCoreClock(72000000);
		LL_SYSTICK_EnableIT();

    /* ADC clock */
    LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSRC_PCLK2_DIV_6); /* 72/6=12MHz */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

void Error_Handler(void)
{
    __disable_irq();
    all_outputs_safe();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
