---
title: Discharge driver FSM - TIM1 (discharge.c)
---
stateDiagram-v2
    direction LR

    [*] --> DIS_IDLE : Discharge_Init() / tim1_stop()

    DIS_IDLE : IDLE
    DIS_IDLE : dis_busy = 0
    DIS_IDLE : timer stopped, MOE = 0
    DIS_IDLE : CH2 mode = ACTIVE

    DIS_ONCE : RUNNING SINGLE
    DIS_ONCE : dis_busy = 1
    DIS_ONCE : tipo_descarga = 0

    DIS_TRAIN : RUNNING TRAIN
    DIS_TRAIN : dis_busy = 1
    DIS_TRAIN : tipo_descarga = 1
    DIS_TRAIN : cont_pulsos counts pulses

    DIS_IDLE --> DIS_ONCE : Discharge_RunCycle_Once() / tim1_fire()
    DIS_IDLE --> DIS_TRAIN : Discharge_RunCycle_Train() / cont_pulsos = 0, tim1_fire()

    DIS_ONCE --> DIS_IDLE : CC3 event / tim1_stop(), dis_busy = 0

    DIS_TRAIN --> DIS_TRAIN : CC3 event, cont_pulsos below NUM_PULSES / re-arm CH2 (OCMODE_ACTIVE)
    DIS_TRAIN --> DIS_IDLE : CC3 event, cont_pulsos reached NUM_PULSES / tim1_stop(), dis_busy = 0

    note right of DIS_TRAIN
        Inside every pulse (1 tick = 1 us)
        0 : CH1 HIGH, CH1N complementary
        CCR2 (11) : CH2 HIGH
        CCR1 (23) : CH1 LOW
        CC4 (50) : ISR forces CH2 LOW (FORCED_INACTIVE)
        CC3 (59) : end of pulse event
        ARR (60) : counter wraps
    end note
