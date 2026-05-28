#include "i2c.h"
#include "bq76952_protocol.h"
#include "stm32g4xx_hal.h"
#include <string.h>

I2C_HandleTypeDef hi2c1;

void MX_I2C1_Init(void) {
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10805E89; // TODO
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    I2C_Error_Handler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
    I2C_Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
    I2C_Error_Handler();
  }

  HAL_I2CEx_EnableFastModePlus(I2C_FASTMODEPLUS_I2C1);
}

bool I2C_Read(uint8_t reg, uint8_t *data, uint16_t size, uint32_t timeout) {
  if (HAL_I2C_Master_Transmit(&hi2c1, TI_BQ_I2C_ADDRESS, &reg, 1, timeout) !=
      HAL_OK) {
    return false;
  }

  if (HAL_I2C_Master_Receive(&hi2c1, TI_BQ_I2C_ADDRESS, data, size, timeout) !=
      HAL_OK) {
    return false;
  }
  return true;
}

bool I2C_Write(uint8_t reg, uint8_t *data, uint16_t size, uint32_t timeout) {
  uint8_t buf[33];

  if (size > 32) {
    return false;
  }

  buf[0] = reg;

  memcpy(&buf[1], data, size);

  if (HAL_I2C_Master_Transmit(&hi2c1, TI_BQ_I2C_ADDRESS, buf, size + 1,
                              timeout) != HAL_OK) {
    return false;
  }

  return true;
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *I2cHandle) {
  if (HAL_I2C_GetError(I2cHandle) != HAL_I2C_ERROR_AF) {
    I2C_Error_Handler();
  }
}

void I2C_Error_Handler(void) {
  while (1) {
  }
}
