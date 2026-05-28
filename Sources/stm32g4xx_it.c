#include "stm32g4xx_it.h"
#include "fdcan.h"
#include "i2c.h"
#include "stm32g4xx_hal.h"

extern I2C_HandleTypeDef hi2c1;
extern FDCAN_HandleTypeDef hfdcan2;

void NMI_Handler(void) {}

void HardFault_Handler(void) {
  while (1) {
  }
}

void SVC_Handler(void) {}

void PendSV_Handler(void) {}

void SysTick_Handler(void) { HAL_IncTick(); }

void I2C1_EV_IRQHandler(void) { HAL_I2C_EV_IRQHandler(&hi2c1); }

void I2C1_ER_IRQHandler(void) { HAL_I2C_ER_IRQHandler(&hi2c1); }

void FDCAN2_IT0_IRQHandler(void) { HAL_FDCAN_IRQHandler(&hfdcan2); }
