---
title: Stepper driver FSM - TIM2 update IRQ (stepper.c)
---
stateDiagram-v2
    direction LR

    [*] --> STEP_IDLE : Stepper_Init() / Stepper_Stop()

    STEP_IDLE : IDLE
    STEP_IDLE : step_busy = 0
    STEP_IDLE : TIM2 stopped, STEP low
    STEP_IDLE : completed_steps = 0

    STEP_LOW : STEP LOW phase
    STEP_LOW : step_busy = 1
    STEP_LOW : step_state = 0

    STEP_HIGH : STEP HIGH phase
    STEP_HIGH : step_busy = 1
    STEP_HIGH : step_state = 1

    STEP_IDLE --> STEP_LOW : Stepper_Mov_Slow / Mov_Mid / Mov_Fast - set planned_steps and DIR, start TIM2

    STEP_LOW --> STEP_HIGH : TIM2 update, completed_steps below planned_steps / step_high(), completed_steps++
    STEP_HIGH --> STEP_LOW : TIM2 update, completed_steps below planned_steps / step_low()
    STEP_HIGH --> STEP_IDLE : TIM2 update, completed_steps reached planned_steps / step_low(), Stepper_Stop()

    STEP_LOW --> STEP_IDLE : Stepper_Stop() called externally
    STEP_HIGH --> STEP_IDLE : Stepper_Stop() called externally

    note right of STEP_IDLE
        Move sizes (planned_steps)
        Mov_Slow : NUM_APROX_STEPS = 1
        Mov_Mid : NUM_STEPS_MID = 50
        Mov_Rapido : NUM_QUICK_STEPS = 35000
        Phase time = TIME_BETWEEN_STEPS ticks
        1 step = 2 TIM2 interrupts
        Calls while busy return 0 and are ignored
    end note
