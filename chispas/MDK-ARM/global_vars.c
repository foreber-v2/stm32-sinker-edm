#include "global_vars.h"

// Aquí se reserva el espacio físico en memoria
volatile uint32_t sys_tick_ms = 0;
volatile uint8_t btn_go_pressed = 0;
volatile uint8_t btn_a_pressed = 0;
volatile uint8_t btn_b_pressed = 0;
volatile uint32_t I_Limit_Max = 0;
volatile uint32_t shorts = 0;
volatile uint32_t sparks = 0;
volatile uint32_t completed_steps = 0;
