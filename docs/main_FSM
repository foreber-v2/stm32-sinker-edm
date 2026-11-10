---
title: Main FSM - Blue Pill STM32F103C8 (main.c)
---
stateDiagram-v2
    direction TB

    [*] --> STATE_IDLE : power-up / all_outputs_safe()

    STATE_IDLE : IDLE
    STATE_IDLE : green LED blinks 500 ms
    STATE_IDLE : relays OFF

    STATE_RELAY_ON : RELAY_ON
    STATE_RELAY_ON : master and charge relays ON
    STATE_RELAY_ON : red LED steady, bleeder OFF

    STATE_WAIT_XMS : WAIT_XMS
    STATE_WAIT_XMS : charge relay released
    STATE_WAIT_XMS : buttons ignored

    STATE_READY : READY
    STATE_READY : green LED steady
    STATE_READY : sparks = 0, shorts = 0

    STATE_READING : READING
    STATE_READING : evaluate sparks and shorts
    STATE_READING : decide step or discharge

    STATE_FORWARD : FORWARD
    STATE_FORWARD : stepper moving down

    STATE_BACKWARD : BACKWARD
    STATE_BACKWARD : stepper moving up

    STATE_DISCHARGE_CONT : DISCHARGE_CONT
    STATE_DISCHARGE_CONT : single or train discharge running

    STATE_DISCHARGE_ONCE : DISCHARGE_ONCE
    STATE_DISCHARGE_ONCE : debug path only

    STATE_BLEEDING : BLEEDING
    STATE_BLEEDING : relays OFF, bleeder ON
    STATE_BLEEDING : safe LED blinks 200 ms

    STATE_LOCKOUT : LOCKOUT
    STATE_LOCKOUT : red LED blinks 200 ms
    STATE_LOCKOUT : I_Limit_Max = 0, buttons ignored

    %% Power-up sequence
    STATE_IDLE --> STATE_RELAY_ON : btn_go_pressed
    STATE_RELAY_ON --> STATE_WAIT_XMS : 8000 ms elapsed
    STATE_WAIT_XMS --> STATE_READY : 100 ms elapsed

    %% Ready branches
    STATE_READY --> STATE_READING : btn_go_pressed
    STATE_READY --> STATE_LOCKOUT : btn_a_pressed / Stepper_Mov_Rapido(SUBE)
    STATE_READY --> STATE_BLEEDING : btn_b_pressed / master relay OFF

    %% Machining loop
    STATE_READING --> STATE_READY : down_steps_done above 3500
    STATE_READING --> STATE_DISCHARGE_CONT : sparks above 1 and sparks above shorts / Discharge_RunCycle_Train()
    STATE_READING --> STATE_FORWARD : no sparks and no shorts / Stepper_Aprox_Inicial(BAJA)
    STATE_READING --> STATE_BACKWARD : otherwise / Stepper_Mov_Mid(SUBE)

    STATE_FORWARD --> STATE_DISCHARGE_CONT : stepper idle / Discharge_RunCycle_Once()
    STATE_BACKWARD --> STATE_DISCHARGE_CONT : stepper idle / Discharge_RunCycle_Once()

    STATE_DISCHARGE_CONT --> STATE_READING : discharge idle
    STATE_DISCHARGE_CONT --> STATE_LOCKOUT : discharge idle and btn_b_pressed

    STATE_DISCHARGE_ONCE --> STATE_READING : discharge idle

    %% Exit paths
    STATE_LOCKOUT --> STATE_READY : 500 ms elapsed
    STATE_BLEEDING --> STATE_IDLE : 8000 ms elapsed

    %% Fallback
    note right of STATE_IDLE
        default case in switch:
        all_outputs_safe() then IDLE
    end note
