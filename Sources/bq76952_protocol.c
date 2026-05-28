
#include "bq76952_protocol.h"
#include "i2c.h"
#include "main.h"
#include "stm32g4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  uint32_t period_ms;
  uint32_t last_run_ms;
  void (*task)(void);
} task_t;

static void task_1hz(void);
static void task_5hz(void);
static void task_10hz(void);

static task_t loop_tasks[] = {
    {1000ULL, 0, task_1hz},
    {200ULL, 0, task_5hz},
    {100ULL, 0, task_10hz},
};
static const size_t loop_task_count =
    sizeof(loop_tasks) / sizeof(loop_tasks[0]);

bq76952_data_t bq76952_data;
extern I2C_HandleTypeDef hi2c1;

bool BQ76952_GetInternalTemp(float *temp) {
  if (temp == NULL) {
    return false;
  }
  unsigned char rx[2] = {0x00};
  if (!BQ76952_DirectCommand(CMD_INTERNAL_TEMP, 0x00, READ, rx)) {
    return false;
  }
  uint16_t temp_raw = (rx[1] << 8) | rx[0];
  *temp = ((float)temp_raw / 10.0f) - 273.15f;
  return true;
}

bool BQ76952_GetCellVoltage(uint8_t cell, uint16_t *mv) {
  if (mv == NULL) {
    return false;
  }
  if (cell == 0 || cell > 16) {
    return false;
  }
  uint8_t rx[2];
  uint8_t reg = CMD_CELL1_VOLTAGE + ((cell - 1) * 2);
  if (!BQ76952_DirectCommand(reg, 0x00, READ, rx)) {
    return false;
  }
  *mv = ((uint16_t)rx[1] << 8) | rx[0];

  return true;
}

bool BQ76952_Subcommand(uint16_t cmd, uint16_t data, uint8_t type,
                        unsigned char *rx_result) {
  uint8_t TX_Reg[4] = {0x00, 0x00, 0x00, 0x00};
  // TX_Reg in little endian format
  TX_Reg[0] = cmd & 0xff;
  TX_Reg[1] = (cmd >> 8) & 0xff;

  if (type == READ) {
    unsigned char RX_32Byte[32] = {0x00};
    if (!I2C_Write(0x3E, TX_Reg, 2, 100)) {
      return false;
    }
    HAL_Delay(20);
    if (!I2C_Read(0x40, RX_32Byte, 32, 100)) {
      return false;
    }
    if (rx_result != NULL) {
      memcpy(rx_result, RX_32Byte, sizeof(RX_32Byte));
    }
  }
  if (type == WRITE) {
    // TODO
  }
  return true;
}

bool BQ76952_DirectCommand(uint16_t cmd, uint16_t data, uint8_t type,
                           unsigned char *rx_result) {
  uint8_t TX_data[2] = {0x00, 0x00};

  // little endian format
  TX_data[0] = data & 0xff;
  TX_data[1] = (data >> 8) & 0xff;
  unsigned char RX_data[2] = {0x00};
  if (type == READ) { // Read
    if (!I2C_Read(cmd, RX_data, 2, 100)) {
      return false;
    }
    if (rx_result != NULL) {
      memcpy(rx_result, RX_data, sizeof(RX_data));
    }
  }
  if (type == WRITE) {
    if (!I2C_Write(cmd, TX_data, 2, 100)) {
      return false;
    }
  }
  return true;
}

static void task_1hz(void) {
  float temp = 0.0f;
  if (BQ76952_GetInternalTemp(&temp)) {
    bq76952_data.internal_temp_c = temp;
    printf("Internal Temp %.2f C\r\n", bq76952_data.internal_temp_c);
  }
}

static void task_5hz(void) {
  // TODO
}

static void task_10hz(void) {
  for (int i = 1; i <= 16; i++) {
    uint16_t mv = 0;
    if (BQ76952_GetCellVoltage(i, &mv)) {
      bq76952_data.cell_mv[i - 1] = mv;
      // printf("Cell[%d]: %u\r\n", i, mv);
    }
  }
}

void BQ76952_Loop() {
  uint32_t now = get_uptime_ms();
  for (uint32_t i = 0; i < loop_task_count; i++) {
    if ((now - loop_tasks[i].last_run_ms) >= loop_tasks[i].period_ms) {
      loop_tasks[i].last_run_ms += loop_tasks[i].period_ms;
      loop_tasks[i].task();
    }
  }
}
