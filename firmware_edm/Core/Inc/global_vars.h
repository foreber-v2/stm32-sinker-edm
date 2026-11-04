#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

#include <stdint.h>

// Declaraciones extern (aviso para otros archivos)
extern volatile uint32_t sys_tick_ms;
extern volatile uint8_t btn_go_pressed; 
extern volatile uint8_t btn_a_pressed;
extern volatile uint8_t btn_b_pressed;
extern volatile uint32_t I_Limit_Max;
extern volatile uint32_t shorts;
extern volatile uint32_t sparks;
extern volatile uint32_t completed_steps;

#endif // GLOBAL_VARS_H