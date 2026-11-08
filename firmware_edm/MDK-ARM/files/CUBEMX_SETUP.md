# CubeMX — Configuración paso a paso para Blue Pill

## 1. Nuevo proyecto
- Board: STM32F103C8Tx (o buscar directamente)
- Toolchain: MDK-ARM (Keil)
- Firmware: STM32Cube FW_F1

## 2. Clock — liberar PB3/PB4 del JTAG
- Pestaña **SYS**
  - Debug: **Serial Wire** (esto libera PB3 y PB4)

## 3. Clock — HSE + PLL a 72 MHz
- Pestaña **RCC**
  - High Speed Clock (HSE): **Crystal/Ceramic Resonator**
- Pestaña **Clock Configuration**
  - Input: HSE 8 MHz
  - PLL Source: HSE
  - PLL Mul: x9
  - System Clock: PLLCLK
  - AHB: 72 MHz
  - APB1: 36 MHz (max)
  - APB2: 72 MHz

## 4. GPIO — salidas y entradas
| Pin | Función | Configuración |
|-----|---------|---------------|
| PA4 | GPIO_Input | Pull-up, label: BTN_MAIN |
| PB3 | GPIO_Output | Push-pull, Low, label: LED_RED |
| PB4 | GPIO_Output | Push-pull, Low, label: LED_GREEN |
| PB5 | GPIO_Output | Push-pull, Low, label: RELAY_MASTER |

## 5. TIM1 — PWM con complementario
- Pestaña **Timers → TIM1**
  - Clock Source: **Internal Clock**
  - Channel 1: **PWM Generation CH1 and CH1N**
  - Channel 2: **PWM Generation CH2**
- Pestaña **Parameter Settings**:
  - Prescaler: 0  (72MHz / 1 = 72MHz → 1 tick = ~13.9 ns)
  - Counter Mode: Up
  - Counter Period (ARR): 65535  (se ajusta en discharge.c)
  - PWM Mode CH1: PWM mode 1
  - PWM Mode CH2: PWM mode 1
  - Pulse CH1: 0
  - Pulse CH2: 0
  - **Dead Time**: 36  (36 ticks × 13.9ns ≈ 500 ns)
  - Automatic Output Enable: **Enable**
  - Break State: **Disable** (por ahora; futuro: Enable con BKIN)
  - Off State Run: **0** (salidas en bajo cuando MOE=0)
  - Lock Level: **Off**
- Pines generados automáticamente:
  - PA8 → TIM1_CH1
  - PA9 → TIM1_CH2
  - PB13 → TIM1_CH1N

## 6. Proyecto
- Pestaña **Project Manager**
  - Project Name: bluepill_discharge
  - Project Location: tu carpeta
  - Toolchain: MDK-ARM V5
- Advanced Settings:
  - Driver selector: **LL** para todos los periféricos usados
  - Marcar: **Generate peripheral initialization as a pair of .c/.h files**

## 7. Generar
- Click **Generate Code**
- Abrir en Keil: bluepill_discharge.uvprojx
