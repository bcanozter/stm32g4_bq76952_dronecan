#ifndef __I2C_H
#define __I2C_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
void MX_I2C1_Init(void);
void I2C_Error_Handler(void);
bool I2C_Write(uint8_t reg, uint8_t *data, uint16_t size, uint32_t timeout);
bool I2C_Read(uint8_t reg, uint8_t *data, uint16_t size, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H */
