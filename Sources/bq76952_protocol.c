#include "bq76952_protocol.h"
#include "i2c.h"
#include "main.h"
#include "stm32g4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define I2C_READ_TIMEOUT_MS 100U
#define I2C_WRITE_TIMEOUT_MS 100U

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

bool BQ76952_DataRAM_Read(uint16_t reg, uint8_t *data, uint8_t len) {
  uint8_t reg_addr[2];

  reg_addr[0] = reg & 0xFF;
  reg_addr[1] = (reg >> 8) & 0xFF;

  if (!I2C_Write(0x3E, reg_addr, 2, I2C_WRITE_TIMEOUT_MS)) {
    return false;
  }
  // This small delay is needed otherwise the read is incorrect.
  HAL_Delay(2);
  if (!I2C_Read(0x40, data, len, I2C_READ_TIMEOUT_MS)) {
    return false;
  }

  return true;
}

bool BQ76952_DataRAM_Write(uint16_t reg, const uint8_t *data, uint8_t len) {
  uint8_t tx[34];
  uint8_t footer[2];

  tx[0] = reg & 0xFF;
  tx[1] = (reg >> 8) & 0xFF;

  memcpy(&tx[2], data, len);

  if (!I2C_Write(0x3E, tx, len + 2, I2C_WRITE_TIMEOUT_MS)) {
    return false;
  }

  footer[0] = BQ76952_Checksum(tx, len + 2);
  footer[1] = len + 4;

  if (!I2C_Write(0x60, footer, 2, I2C_WRITE_TIMEOUT_MS)) {
    return false;
  }

  HAL_Delay(2);

  return true;
}

// https://github.com/TexasInstruments/mspm0-sdk/blob/main/examples/nortos/LP_MSPM0G3519/demos/bq769x2_TIDA010247/BQ769x2_Configs/BQ769x2_protocol.c#L130
uint8_t BQ76952_Checksum(const uint8_t *data, uint8_t len) {
  unsigned char i;
  unsigned char checksum = 0;

  for (i = 0; i < len; i++)
    checksum += data[i];

  checksum = 0xff & ~checksum;

  return (checksum);
}

// https://github.com/TexasInstruments/mspm0-sdk/blob/main/examples/nortos/LP_MSPM0G3519/demos/bq769x2_TIDA010247/BQ769x2_Configs/BQ769x2_protocol.c#L143
uint8_t BQ76952_CRC8(const uint8_t *data, uint8_t len) {
  // Not used for now. CRC Mode is `disabled` at the moment.
  unsigned char i;
  unsigned char crc = 0;
  while (len-- != 0) {
    for (i = 0x80; i != 0; i /= 2) {
      if ((crc & 0x80) != 0) {
        crc *= 2;
        crc ^= 0x107;
      } else
        crc *= 2;

      if ((*data & i) != 0)
        crc ^= 0x107;
    }
    data++;
  }
  return (crc);
}

static void BQ76952_TestRAMRead(void) {
  uint8_t value;
  if (!BQ76952_DataRAM_Read(REG_ENABLED_PROTECTIONS_A, &value, 1)) {
    printf("RAM read failed\r\n");
    return;
  }
  // Default value is 0x88
  // https://www.ti.com/lit/an/sluaa11b/sluaa11b.pdf
  printf("Enabled Protections A = 0x%02X\r\n", value);
}

bool BQ76952_TestRAMWrite(void) {
  uint8_t original;
  uint8_t modified;
  uint8_t verify;

  if (!BQ76952_DataRAM_Read(REG_ENABLED_PROTECTIONS_A, &original, 1)) {
    printf("RAM read failed\r\n");
    return false;
  }

  printf("Original = 0x%02X\r\n", original);

  // Just an example.
  modified = 0x8C;

  if (!BQ76952_DataRAM_Write(REG_ENABLED_PROTECTIONS_A, &modified, 1)) {
    printf("RAM write failed\r\n");
    return false;
  }

  HAL_Delay(10);

  if (!BQ76952_DataRAM_Read(REG_ENABLED_PROTECTIONS_A, &verify, 1)) {
    printf("RAM verify read failed\r\n");
    return false;
  }

  printf("Verify = 0x%02X\r\n", verify);
  if (verify != modified) {
    printf("RAM verify mismatch\r\n");
    return false;
  }

  if (!BQ76952_DataRAM_Write(REG_ENABLED_PROTECTIONS_A, &original, 1)) {
    printf("RAM restore failed\r\n");
    return false;
  }
  printf("RAM write verified\r\n");

  return true;
}

bool BQ76952_Subcommand(uint16_t cmd, uint16_t data, uint8_t type,
                        unsigned char *rx_result, uint8_t rx_len) {
  uint8_t TX_Reg[4] = {0x00, 0x00, 0x00, 0x00};
  // TX_Reg in little endian format
  TX_Reg[0] = cmd & 0xff;
  TX_Reg[1] = (cmd >> 8) & 0xff;

  if (type == READ) {
    unsigned char RX_32Byte[32] = {0x00};
    if (!I2C_Write(0x3E, TX_Reg, 2, I2C_WRITE_TIMEOUT_MS)) {
      return false;
    }
    if (rx_len == 0 || rx_len > 32) {
      return false;
    }
    HAL_Delay(5);
    if (!I2C_Read(0x40, RX_32Byte, rx_len, I2C_READ_TIMEOUT_MS)) {
      return false;
    }
    if (rx_result != NULL) {
      memcpy(rx_result, RX_32Byte, rx_len);
    }
  }
  if (type == WRITE) {
    if (!I2C_Write(0x3E, TX_Reg, 2, I2C_WRITE_TIMEOUT_MS)) {
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
    if (!I2C_Read(cmd, RX_data, 2, I2C_READ_TIMEOUT_MS)) {
      return false;
    }
    if (rx_result != NULL) {
      memcpy(rx_result, RX_data, sizeof(RX_data));
    }
  }
  if (type == WRITE) {
    if (!I2C_Write(cmd, TX_data, 2, I2C_WRITE_TIMEOUT_MS)) {
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
  BQ76952_TestRAMRead();
  BQ76952_TestRAMWrite();
  while (1) {
    BQ76952_ExitConfigUpdateMode();
    HAL_Delay(2);
    BQ76952_GetBatteryStatus(&bq76952_data.battery_status);
    if ((bq76952_data.battery_status & BIT_CFGUPDATE) == 0) {
      printf("CFGUPDATE exit\r\n");
      break;
    }
    HAL_Delay(10);
  }
}

void BQ76952_loop() {
  uint32_t now = get_uptime_ms();
  for (uint32_t i = 0; i < loop_task_count; i++) {
    if ((now - loop_tasks[i].last_run_ms) >= loop_tasks[i].period_ms) {
      loop_tasks[i].last_run_ms += loop_tasks[i].period_ms;
      loop_tasks[i].task();
    }
  }
}
