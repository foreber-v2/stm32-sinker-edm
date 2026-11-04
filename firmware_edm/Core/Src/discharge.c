#include "discharge.h"
#include "tim.h"
#include "stm32f1xx_ll_tim.h"
#include "global_vars.h" 


static volatile uint8_t dis_busy = 0;
static volatile uint8_t tipo_descarga = 0; // opciones 0 = unica, 1 = tren
static volatile uint32_t cont_pulsos = 0;	 // cuenta cuantos pulsos se han cumplido


/* ── tim1_stop ───────────────────────────────────────────────────────── */
static void tim1_stop(void)
{
    LL_TIM_DisableCounter(TIM1);
		LL_TIM_DisableIT_CC1(TIM1);
    LL_TIM_ClearFlag_CC1(TIM1);
		LL_TIM_DisableIT_CC2(TIM1);
    LL_TIM_ClearFlag_CC2(TIM1);
    LL_TIM_DisableIT_CC3(TIM1);
    LL_TIM_ClearFlag_CC3(TIM1);
		LL_TIM_DisableIT_CC4(TIM1);
    LL_TIM_ClearFlag_CC4(TIM1);
    LL_TIM_ClearFlag_UPDATE(TIM1);
		LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_ACTIVE);
    LL_TIM_DisableAllOutputs(TIM1);  // MOE=0 → pines a idle state
}

/* ── tim1_fire ───────────────────────────────────────────────────────── */
/*
 * Configura y arranca UN pulso:
 *   arr:  duracion total del tramo en ticks (1 tick = 1us)
 *   ccr1: punto de corte CH1/CH1N
 *   ccr2: punto de corte CH2
 *   ccr3: tick en que CC3 dispara para parar antes de ARR
 *           → poner arr-1 para que pare 1 tick antes del UIF
 */
static void tim1_fire(uint32_t arr, uint32_t ccr1, uint32_t ccr2, uint32_t ccr4, uint32_t ccr3)
{
    LL_TIM_DisableCounter(TIM1);
    LL_TIM_SetCounter(TIM1, 0);
    LL_TIM_SetAutoReload(TIM1, arr);
    LL_TIM_OC_SetCompareCH1(TIM1, ccr1);
    LL_TIM_OC_SetCompareCH2(TIM1, ccr2);
    LL_TIM_OC_SetCompareCH3(TIM1, ccr3);
		LL_TIM_OC_SetCompareCH4(TIM1, ccr4);  

		LL_TIM_ClearFlag_CC1(TIM1);
	  LL_TIM_ClearFlag_CC2(TIM1);
    LL_TIM_ClearFlag_CC3(TIM1);
	  LL_TIM_ClearFlag_CC4(TIM1);
    LL_TIM_ClearFlag_UPDATE(TIM1);

    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1 | LL_TIM_CHANNEL_CH1N);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
		
		LL_TIM_EnableIT_CC1(TIM1);              
		LL_TIM_EnableIT_CC2(TIM1);
    LL_TIM_EnableIT_CC3(TIM1);              
		LL_TIM_EnableIT_CC4(TIM1);
    LL_TIM_EnableAllOutputs(TIM1);          // MOE=1
    LL_TIM_EnableCounter(TIM1);
}

/* ── API pública ─────────────────────────────────────────────────────── */

void Discharge_Init(void)
{
    LL_TIM_DisableCounter(TIM1);
    LL_TIM_EnableARRPreload(TIM1);

    // Valores seguros: CCR > ARR → CH1 y CH2 nunca HIGH
    LL_TIM_SetAutoReload(TIM1, 65535);
    LL_TIM_OC_SetCompareCH1(TIM1, 65535);
    LL_TIM_OC_SetCompareCH2(TIM1, 65535);
    LL_TIM_OC_SetCompareCH3(TIM1, 65535);
		LL_TIM_OC_SetCompareCH4(TIM1, 65535);
	
    // Solo TIM1_CC_IRQn — el UIF no se usa
    NVIC_SetPriority(TIM1_CC_IRQn, 0);
    NVIC_EnableIRQ(TIM1_CC_IRQn);

    tim1_stop();
    dis_busy = 0;
}

uint8_t Discharge_IsBusy(void)
{
    return dis_busy;
}

uint8_t Discharge_RunCycle_Train(void)
{
    if (dis_busy) return 1;
	
		tipo_descarga = 1;		
    dis_busy = 1;
		cont_pulsos = 0;


    tim1_fire(
        T_TOTAL_US,          // ARR
        T_CH1_TO_LOW_US,       // Fin CH1 ccr1
        T_CH2_TO_HIGH_US,       // Inicio CH2 ccr2
        T_CH2_TO_LOW_US,       // Fin CH2 ccr4
				T_TOTAL_US - 1			 // CCR3: parada anticipada ccr3
    );
		
		
    return 1;
}

uint8_t Discharge_RunCycle_Unico(void)
{
    if (dis_busy) return 0;
		
		tipo_descarga = 0;
    dis_busy = 1;

    /*
     * Un solo tramo:
     *   ARR  = DIS_PWM_US        → duración total
     *   CCR1 = CCR1_PWMN1_US     → CH1 HIGH desde tick DIS_PWMN1_US
     *                              CH1N HIGH antes de ese tick (complementario)
     *   CCR2 = CRR2_PWMN2_US     → CH2 HIGH desde tick CRR2_PWMN2_US
		 *	 CCR3 = CRR3_PWMN2_US			→ CH2 LOW desde tick CRR3_PWMN2_US
     *   CCR4 = ARR_PWM_US - 1	 	→ CC4 dispara 1 tick antes de ARR
     *                              ISR para el timer limpiamente
     */
    tim1_fire(
        T_TOTAL_US,          // ARR
        T_CH1_TO_LOW_US,       // Fin CH1
        T_CH2_TO_HIGH_US,       // Inicio CH2
        T_CH2_TO_LOW_US,       // Fin CH2
				T_TOTAL_US - 1			 // CCR3: parada anticipada 
    );
		
		

    return 1;
}

void Discharge_IRQHandler(void)
{
		if (LL_TIM_IsActiveFlag_CC4(TIM1)) // se activa primero, ccr4 va antes que ccr3
		{
			LL_TIM_ClearFlag_CC4(TIM1);                 
			LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_FORCED_INACTIVE); // apaga el channel 2 forzado
			dis_busy = 1;
		}
		
    if (LL_TIM_IsActiveFlag_CC3(TIM1))  
		{
			
			LL_TIM_ClearFlag_CC3(TIM1);                  
			
			if (tipo_descarga == 0) 
			{
				
				tim1_stop();       // para contador, MOE=0 → pines a idle state seguro
				dis_busy = 0;				
			}
			
			else if (tipo_descarga == 1) 
			{
				
				cont_pulsos++;
				
				if (cont_pulsos < NUM_PULSES) 
				{
					LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_ACTIVE); // vuelve a activar channel 2 por haberlo apagado forzado
					dis_busy = 1;
				}
				
				else 
				{
					tim1_stop();       // para contador, MOE=0 = pines a idle state seguro
					dis_busy = 0;
					cont_pulsos = 0;
				}			
			}
		}
		
}