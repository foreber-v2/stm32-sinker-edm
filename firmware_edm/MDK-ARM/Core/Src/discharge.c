/**
 ******************************************************************************
 * @file    discharge.c
 * @brief   Discharge cycle driver using TIM1 (single pulse or pulse train)
 ******************************************************************************
 *
 * INTERNAL STATE MACHINE (full diagram in discharge_fsm.mermaid)
 *
 *   IDLE --Once()---> RUNNING (tipo_descarga = 0) --CC3--> IDLE
 *   IDLE --Train()--> RUNNING (tipo_descarga = 1) --CC3, cont_pulsos < NUM_PULSES--> RUNNING (next pulse)
 *                                                 --CC3, cont_pulsos = NUM_PULSES--> IDLE
 *
 * PER-PULSE EVENTS (1 tick = 1 us)
 *   CH1 / CH1N : PWM1, HIGH until CCR1 (T_CH1_TO_LOW_US)
 *   CH2        : "Active on match": HIGH at CCR2 (T_CH2_TO_HIGH_US)
 *   CC4 event  : (T_CH2_TO_LOW_US) ISR forces CH2 LOW
 *   CC3 event  : (T_TOTAL_US - 1)  ISR stops the timer, or re-arms CH2 for the next pulse
 *
 * WHY CH2 NEEDS RE-ARMING
 *   In "Active on level match" mode the output only goes HIGH on a compare match,
 *   it never goes LOW by itself. So the ISR ends the CH2 pulse by switching
 *   the channel to FORCED_INACTIVE (CC4). While in that mode the output stays
 *   LOW and ignores compare matches, so it must be switched back to
 *   OCMODE_ACTIVE before the next pulse, otherwise CH2 would never go HIGH
 *   again. That is what the OCMODE_ACTIVE call in the CC3 branch does.
 ******************************************************************************
 */

#include "discharge.h"
#include "tim.h"
#include "stm32f1xx_ll_tim.h"
#include "global_vars.h"

/* -- Private variables ------------------------------------------------------ */
static volatile uint8_t  dis_busy      = 0;   /* 1 = discharge cycle running                  */
static volatile uint8_t  tipo_descarga = 0;   /* Discharge type: 0 = single, 1 = train        */
static volatile uint32_t cont_pulsos   = 0;   /* Number of pulses completed in the train      */

/* -- Private functions ------------------------------------------------------ */

/**
 * @brief  Stops TIM1 and leaves everything in a safe state:
 *         counter off, CC interrupts off, flags cleared,
 *         CH2 re-armed (ACTIVE mode) and MOE = 0 (pins go to idle state).
 */
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

    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_ACTIVE);   /* Re-arm CH2 for the next cycle */
    LL_TIM_DisableAllOutputs(TIM1);                                      /* MOE = 0 -> pins to idle state */
}

/**
 * @brief  Configures and starts ONE pulse.
 * @param  arr   Total pulse duration in ticks (1 tick = 1 us)
 * @param  ccr1  CH1 / CH1N cut point
 * @param  ccr2  CH2 start point (goes HIGH)
 * @param  ccr4  CH2 end point (ISR forces LOW)
 * @param  ccr3  Tick at which CC3 fires to stop before ARR
 *               (use arr - 1 to stop 1 tick before the update event)
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

    /* Clear stale flags */
    LL_TIM_ClearFlag_CC1(TIM1);
    LL_TIM_ClearFlag_CC2(TIM1);
    LL_TIM_ClearFlag_CC3(TIM1);
    LL_TIM_ClearFlag_CC4(TIM1);
    LL_TIM_ClearFlag_UPDATE(TIM1);

    /* Enable output channels */
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1 | LL_TIM_CHANNEL_CH1N);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);

    /* Enable compare interrupts */
    LL_TIM_EnableIT_CC1(TIM1);
    LL_TIM_EnableIT_CC2(TIM1);
    LL_TIM_EnableIT_CC3(TIM1);
    LL_TIM_EnableIT_CC4(TIM1);

    LL_TIM_EnableAllOutputs(TIM1);   /* MOE = 1 */
    LL_TIM_EnableCounter(TIM1);
}

/* -- Public API ------------------------------------------------------------- */

void Discharge_Init(void)
{
    LL_TIM_DisableCounter(TIM1);
    LL_TIM_EnableARRPreload(TIM1);

    /* Safe values: CCR > ARR -> CH1 and CH2 never go HIGH */
    LL_TIM_SetAutoReload(TIM1, 65535);
    LL_TIM_OC_SetCompareCH1(TIM1, 65535);
    LL_TIM_OC_SetCompareCH2(TIM1, 65535);
    LL_TIM_OC_SetCompareCH3(TIM1, 65535);
    LL_TIM_OC_SetCompareCH4(TIM1, 65535);

    /* Only TIM1_CC_IRQn is used - the update interrupt (UIF) is not */
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
    if (dis_busy) return 0;   

    tipo_descarga = 1;
    dis_busy      = 1;
    cont_pulsos   = 0;

    tim1_fire(
        T_TOTAL_US,            /* ARR                                      */
        T_CH1_TO_LOW_US,       /* CCR1: end of CH1                         */
        T_CH2_TO_HIGH_US,      /* CCR2: start of CH2                       */
        T_CH2_TO_LOW_US,       /* CCR4: end of CH2                         */
        T_TOTAL_US - 1         /* CCR3: early stop / end-of-pulse event    */
    );

    return 1;
}

uint8_t Discharge_RunCycle_Once(void)
{
    if (dis_busy) return 0;

    tipo_descarga = 0;
    dis_busy      = 1;

    /*
     * Single pulse:
     *   ARR  = T_TOTAL_US        -> total duration
     *   CCR1 = T_CH1_TO_LOW_US   -> CH1 HIGH until this tick
     *                               (CH1N is the complement, with dead-time)
     *   CCR2 = T_CH2_TO_HIGH_US  -> CH2 goes HIGH at this tick
     *   CCR4 = T_CH2_TO_LOW_US   -> ISR forces CH2 LOW at this tick
     *   CCR3 = T_TOTAL_US - 1    -> fires 1 tick before ARR,
     *                               ISR stops the timer cleanly
     */
    tim1_fire(
        T_TOTAL_US,            /* ARR                                      */
        T_CH1_TO_LOW_US,       /* CCR1: end of CH1                         */
        T_CH2_TO_HIGH_US,      /* CCR2: start of CH2                       */
        T_CH2_TO_LOW_US,       /* CCR4: end of CH2                         */
        T_TOTAL_US - 1         /* CCR3: early stop                         */
    );

    return 1;
}

/* -- Interrupt handler (call from TIM1_CC_IRQHandler) ------------------------ */
void Discharge_IRQHandler(void)
{
    /* CC4: end of the CH2 pulse. Fires before CC3 (CCR4 < CCR3). */
    if (LL_TIM_IsActiveFlag_CC4(TIM1))
    {
        LL_TIM_ClearFlag_CC4(TIM1);
        LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_FORCED_INACTIVE);   /* Force channel 2 LOW */
        dis_busy = 1;
    }

    /* CC3: end of pulse (ARR - 1) */
    if (LL_TIM_IsActiveFlag_CC3(TIM1))
    {
        LL_TIM_ClearFlag_CC3(TIM1);

        if (tipo_descarga == 0)
        {
            /* Single pulse finished */
            tim1_stop();   /* Stops counter, MOE = 0 -> pins to safe idle state */
            dis_busy = 0;
        }
        else if (tipo_descarga == 1)
        {
            /* Pulse train */
            cont_pulsos++;

            if (cont_pulsos < NUM_PULSES)
            {
                /* Re-arm channel 2 (it was forced inactive at CC4) for the next pulse */
                LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_ACTIVE);
                dis_busy = 1;
            }
            else
            {
                /* Train finished */
                tim1_stop();   /* Stops counter, MOE = 0 -> pins to safe idle state */
                dis_busy    = 0;
                cont_pulsos = 0;
            }
        }
    }
}