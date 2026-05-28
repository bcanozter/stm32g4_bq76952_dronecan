
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

bool BQ76952_GetDeviceNumber(uint16_t *device_num) {
  uint8_t rx[2];
  if (device_num == NULL) {
    return false;
  }
  if (!BQ76952_Subcommand(SUB_CMD_DEVICE_NUM, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *device_num = (rx[1]) << 8 | rx[0];
  return true;
}

bool BQ76952_GetBatteryStatus(uint16_t *battery_status) {
  uint8_t rx[2] = {0x00};
  if (battery_status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_BATTERY_STATUS, 0x00, READ, rx)) {
    return false;
  }
  *battery_status = (rx[1] << 8) | rx[0];
  return true;
}

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

bool BQ76952_EnterConfigUpdateMode() {
  if (!BQ76952_Subcommand(SET_CFGUPDATE, 0, WRITE, NULL, 0)) {
    return false;
  }
  return true;
}

bool BQ76952_ExitConfigUpdateMode() {
  if (!BQ76952_Subcommand(EXIT_CFGUPDATE, 0, WRITE, NULL, 0)) {
    return false;
  }
  return true;
}

bool BQ76952_Reset() {
  if (!BQ76952_Subcommand(_RESET, 0, WRITE, NULL, 0)) {
    return false;
  }
  return true;
}

void BQ76952_PrintBatteryStatus(uint16_t status) {
  printf("Battery Status: 0x%04X\r\n", status);

  printf("Active flags: ");

  if (status & BIT_CFGUPDATE) {
    printf("CFGUPDATE | ");
  }
  if (status & BIT_PCHG_MODE) {
    printf("PCHG_MODE | ");
  }
  if (status & BIT_SLEEP_EN) {
    printf("SLEEP_EN | ");
  }
  if (status & BIT_POR) {
    printf("POR | ");
  }
  if (status & BIT_WD) {
    printf("WD | ");
  }
  if (status & BIT_COW_CHK) {
    printf("COW_CHK | ");
  }
  if (status & BIT_OTPW) {
    printf("OTPW | ");
  }
  if (status & BIT_OTPB) {
    printf("OTPB | ");
  }

  if (status & BIT_SEC0) {
    printf("SEC0 | ");
  }
  if (status & BIT_SEC1) {
    printf("SEC1 | ");
  }
  if (status & BIT_FUSE) {
    printf("FUSE | ");
  }
  if (status & BIT_SS) {
    printf("SS | ");
  }
  if (status & BIT_PF) {
    printf("PF | ");
  }
  if (status & BIT_SD_CMD) {
    printf("SD_CMD | ");
  }
  if (status & BIT_RSVD_14) {
    printf("RSVD_14 | ");
  }
  if (status & BIT_SLEEP) {
    printf("SLEEP ");
  }

  printf("\r\n");
}

bool BQ76952_Subcommand(uint16_t cmd, uint16_t data, uint8_t type,
                        unsigned char *rx_result, uint8_t rx_len) {
  uint8_t TX_Reg[4] = {0x00, 0x00, 0x00, 0x00};
  // TX_Reg in little endian format
  TX_Reg[0] = cmd & 0xff;
  TX_Reg[1] = (cmd >> 8) & 0xff;

  if (type == READ) {
    unsigned char RX_32Byte[32] = {0x00};
    if (!I2C_Write(0x3E, TX_Reg, 2, 100)) {
      return false;
    }
    if (rx_len == 0 || rx_len > 32) {
      return false;
    }
    HAL_Delay(5);
    if (!I2C_Read(0x40, RX_32Byte, rx_len, 100)) {
      return false;
    }
    if (rx_result != NULL) {
      memcpy(rx_result, RX_32Byte, rx_len);
    }
  }
  if (type == WRITE) {
    if (!I2C_Write(0x3E, TX_Reg, 2, 100)) {
      return false;
    }
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
  BQ76952_GetBatteryStatus(&bq76952_data.battery_status);
  BQ76952_PrintBatteryStatus(bq76952_data.battery_status);
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

void BQ76952_init() {
  memset(&bq76952_data, 0, sizeof(bq76952_data_t));
  if (!BQ76952_GetDeviceNumber(&bq76952_data.device_num)) {
    printf("Device read failed\r\n");
    return;
  }
  printf("Device Number 0x%04x\r\n", bq76952_data.device_num);
  /*
  NOTE:
  When writing to RAM registers, it is highly recommended to first enter
  CONFIG_UPDATE mode and then perform the command to exit CONFIG_UPDATE mode
  once complete. This ensures stable operation while settings are being
  modified.
  */
  BQ76952_EnterConfigUpdateMode();
  while (1) {
    BQ76952_GetBatteryStatus(&bq76952_data.battery_status);
    if (bq76952_data.battery_status & BIT_CFGUPDATE) {
      printf("CFGUPDATE entered\r\n");
      break;
    }
    HAL_Delay(10);
  }
  // TODO
  // BQ76952_Reset();
  BQ76952_ExitConfigUpdateMode();
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
