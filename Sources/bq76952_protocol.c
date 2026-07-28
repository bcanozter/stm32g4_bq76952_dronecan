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

// Extern Definitions
extern I2C_HandleTypeDef hi2c1;

static const size_t loop_task_count =
    sizeof(loop_tasks) / sizeof(loop_tasks[0]);

bq76952_data_t bq76952_data;

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

bool BQ76952_GetManufacturingStatus(uint16_t *status) {
  uint8_t rx[2];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_Subcommand(SUB_CMD_MANUFACTURING_STATUS, 0x00, READ, rx,
                          sizeof(rx))) {
    return false;
  }
  *status = ((uint16_t)rx[1]) << 8 | rx[0];
  return true;
}

bool BQ76952_GetBatteryStatus(uint16_t *battery_status) {
  uint8_t rx[2] = {0x00};
  if (battery_status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_BATTERY_STATUS, 0x00, READ, rx, sizeof(rx))) {
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
  if (!BQ76952_DirectCommand(CMD_INTERNAL_TEMP, 0x00, READ, rx, sizeof(rx))) {
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
  if (!BQ76952_DirectCommand(reg, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *mv = ((uint16_t)rx[1] << 8) | rx[0];

  return true;
}

bool BQ76952_GetStackVoltage(uint16_t *mv)
{
  uint8_t rx[2];
  if (mv == NULL)
  {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_STACK_VOLTAGE, 0x00, READ, rx, sizeof(rx)))
  {
    return false;
  }
  *mv = ((uint16_t)rx[1] << 8) | rx[0];
  return true;
}

bool BQ76952_GetPackPinVoltage(uint16_t *mv)
{
  uint8_t rx[2];
  if (mv == NULL)
  {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_PACK_PIN_VOLTAGE, 0x00, READ, rx, sizeof(rx)))
  {
    return false;
  }
  *mv = ((uint16_t)rx[1] << 8) | rx[0];
  return true;
}

bool BQ76952_GetLDPinVoltage(uint16_t *mv)
{
  uint8_t rx[2];
  if (mv == NULL)
  {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_LD_PIN_VOLTAGE, 0x00, READ, rx, sizeof(rx)))
  {
    return false;
  }
  *mv = ((uint16_t)rx[1] << 8) | rx[0];
  return true;
}

bool BQ76952_GetCC2Current(int16_t *current_ma)
{
  uint8_t rx[2];
  if (current_ma == NULL)
  {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_CC2_CURRENT, 0x00, READ, rx, sizeof(rx)))
  {
    return false;
  }
  *current_ma = (int16_t)(((uint16_t)rx[1] << 8) | rx[0]);
  return true;
}

bool BQ76952_GetFETStatus(uint16_t *status)
{
    uint8_t rx[1];
    if (status == NULL)
    {
        return false;
    }
    if (!BQ76952_DirectCommand(CMD_FET_STATUS, 0x0000, READ, rx, sizeof(rx)))
    {
        return false;
    }
    *status = rx[0];
    return true;
}

bool BQ76952_SetManufacturingStatusBit(uint16_t bit_mask, bool desired_state) {
  uint16_t toggle_subcommand;

  switch (bit_mask) {
  case BIT_MFG_STATUS_FET_EN:
    toggle_subcommand = FET_ENABLE;
    break;
  case BIT_MFG_STATUS_PF_EN:
    toggle_subcommand = PF_ENABLE;
    break;
  case BIT_MFG_STATUS_PDSG_TEST:
    toggle_subcommand = PDSGTEST;
    break;
  case BIT_MFG_STATUS_PCHG_TEST:
    toggle_subcommand = PCHGTEST;
    break;
  case BIT_MFG_STATUS_CHG_TEST:
    toggle_subcommand = CHGTEST;
    break;
  case BIT_MFG_STATUS_DSG_TEST:
    toggle_subcommand = DSGTEST;
    break;
  default:
    return false;
  }

  uint16_t status;
  if (!BQ76952_GetManufacturingStatus(&status)) {
    return false;
  }

  bool current_state = (status & bit_mask) != 0;
  if (current_state == desired_state) {
    return true;
  }

  return BQ76952_Subcommand(toggle_subcommand, 0, WRITE, NULL, 0);
}

bool BQ76952_AllFETsOff(void) {
  return BQ76952_Subcommand(ALL_FETS_OFF, 0, WRITE, NULL, 0);
}

bool BQ76952_AllFETsOn(void) {
  return BQ76952_Subcommand(ALL_FETS_ON, 0, WRITE, NULL, 0);
}

bool BQ76952_SetFETControl(uint8_t fet_control_bits) {
  return BQ76952_DataRAM_Write(SUB_CMD_FET_CONTROL, &fet_control_bits, 1);
}

bool BQ76952_TestFETControl(void) {
  uint16_t status;

  BQ76952_GetFETStatus(&status);
  printf("FET Status before = 0x%04X\r\n", status);

  if (!BQ76952_AllFETsOff()) {
    printf("AllFETsOff failed\r\n");
    return false;
  }
  HAL_Delay(250);
  BQ76952_GetFETStatus(&status);
  printf("FET Status after AllFETsOff = 0x%04X\r\n", status);

  if (!BQ76952_AllFETsOn()) {
    printf("AllFETsOn failed\r\n");
    return false;
  }
  HAL_Delay(250);
  BQ76952_GetFETStatus(&status);
  printf("FET Status after AllFETsOn = 0x%04X\r\n", status);

  return true;
}

bool BQ76952_GetSafetyAlertA(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_SAFETY_ALERT_A, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetSafetyStatusA(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_SAFETY_STATUS_A, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetSafetyAlertB(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_SAFETY_ALERT_B, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetSafetyStatusB(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_SAFETY_STATUS_B, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetSafetyAlertC(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_SAFETY_ALERT_C, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetSafetyStatusC(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_SAFETY_STATUS_C, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetPFAlertA(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_PF_ALERT_A, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetPFStatusA(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_PF_STATUS_A, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetPFAlertB(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_PF_ALERT_B, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetPFStatusB(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_PF_STATUS_B, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetPFAlertC(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_PF_ALERT_C, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetPFStatusC(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_PF_STATUS_C, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetPFAlertD(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_PF_ALERT_D, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetPFStatusD(uint16_t *status) {
  uint8_t rx[1];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_PF_STATUS_D, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = rx[0];
  return true;
}

bool BQ76952_GetAlarmStatus(uint16_t *status) {
  uint8_t rx[2];
  if (status == NULL) {
    return false;
  }
  if (!BQ76952_DirectCommand(CMD_ALARM_STATUS, 0x00, READ, rx, sizeof(rx))) {
    return false;
  }
  *status = ((uint16_t)rx[1] << 8) | rx[0];
  return true;
}

//not tested
bq76952_charge_state_t BQ76952_DetectChargeState(int16_t current_ma,
                                                    bq76952_charge_state_t prev_state) {
  if (current_ma >= BQ_CHARGE_CURRENT_THRESHOLD_MA) {
    return BQ_CHARGE_STATE_CHARGING;
  }
  if (current_ma <= -BQ_DISCHARGE_CURRENT_THRESHOLD_MA) {
    return BQ_CHARGE_STATE_DISCHARGING;
  }
  //dead zone
  if (prev_state == BQ_CHARGE_STATE_CHARGING &&
      current_ma > BQ_CHARGE_CURRENT_THRESHOLD_MA - BQ_CHARGE_STATE_HYSTERESIS_MA) {
    return BQ_CHARGE_STATE_CHARGING;
  }
  if (prev_state == BQ_CHARGE_STATE_DISCHARGING &&
      current_ma < -(BQ_DISCHARGE_CURRENT_THRESHOLD_MA - BQ_CHARGE_STATE_HYSTERESIS_MA)) {
    return BQ_CHARGE_STATE_DISCHARGING;
  }
  return BQ_CHARGE_STATE_IDLE;
}

void BQ76952_UpdateChargeState(void) {
  bq76952_data.charge_state = BQ76952_DetectChargeState(
      bq76952_data.cc2_current_ma, bq76952_data.charge_state);
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
  printf("  Battery Status: 0x%04X\r\n", status);

  printf("  Active flags: ");

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

void BQ76952_PrintManufacturingStatus(uint16_t status) {
  printf("  Manufacturing Status: 0x%04X\r\n", status);

  printf("  Active flags: ");

  if (status & BIT_MFG_STATUS_PCHG_TEST) {
    printf("PCHG_TEST | ");
  }
  if (status & BIT_MFG_STATUS_CHG_TEST) {
    printf("CHG_TEST | ");
  }
  if (status & BIT_MFG_STATUS_DSG_TEST) {
    printf("DSG_TEST | ");
  }
  if (status & BIT_MFG_STATUS_FET_EN) {
    printf("FET_EN | ");
  }
  if (status & BIT_MFG_STATUS_PDSG_TEST) {
    printf("PDSG_TEST | ");
  }
  if (status & BIT_MFG_STATUS_PF_EN) {
    printf("PF_EN | ");
  }
  if (status & BIT_MFG_STATUS_OTPW_EN) {
    printf("OTPW_EN ");
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

bool BQ76952_ConfigureProtections(void) {
  uint8_t vcell_mode[2] = {VCELL_MODE_16S & 0xFF, (VCELL_MODE_16S >> 8) & 0xFF};
  // TODO: protections to be enabled, protections C?
  uint8_t protections_a = ENABLED_PROTECTIONS_A_VALUE;
  uint8_t protections_b = ENABLED_PROTECTIONS_B_VALUE;

  if (!BQ76952_DataRAM_Write(REG_VCELL_MODE, vcell_mode, sizeof(vcell_mode))) {
    printf("Vcell Mode write failed\r\n");
    return false;
  }
  HAL_Delay(2);

  if (!BQ76952_DataRAM_Write(REG_ENABLED_PROTECTIONS_A, &protections_a, 1)) {
    printf("Enabled Protections A write failed\r\n");
    return false;
  }
  HAL_Delay(2);

  if (!BQ76952_DataRAM_Write(REG_ENABLED_PROTECTIONS_B, &protections_b, 1)) {
    printf("Enabled Protections B write failed\r\n");
    return false;
  }
  HAL_Delay(2);

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
                           unsigned char *rx_result, uint8_t len) {
  uint8_t TX_data[2] = {0x00, 0x00};

  // little endian format
  TX_data[0] = data & 0xff;
  TX_data[1] = (data >> 8) & 0xff;
  unsigned char RX_data[2] = {0x00};
  if (type == READ) { // Read
    if (len == 0 || len > sizeof(RX_data)) {
      return false;
    }
    if (!I2C_Read(cmd, RX_data, len, I2C_READ_TIMEOUT_MS)) {
      return false;
    }
    if (rx_result != NULL) {
      memcpy(rx_result, RX_data, len);
    }
  }
  if (type == WRITE) {
    if (!I2C_Write(cmd, TX_data, 2, I2C_WRITE_TIMEOUT_MS)) {
      return false;
    }
  }
  return true;
}

void ReadCellVoltages(void)
{
  int cell_count = 0;
  for (int i = 1; i <= 16; i++)
  {
    uint16_t mv = 0;
    if (BQ76952_GetCellVoltage(i, &mv))
    {
      bq76952_data.cell_mv[i - 1] = mv;
      if (mv > 500U) //MAGIC NUMBER
      {
        cell_count++;
      }
      // printf("Cell[%d]: %u\r\n", i, mv);
    }
    else
    {
      bq76952_data.cell_mv[i - 1] = 0;
    }
  }
  bq76952_data.cell_count = cell_count;
}

static void debug_print(void)
{
  printf("\r\n========== BQ76952 STATUS ==========\r\n");

  printf("Device Number: 0x%04X\r\n",
         bq76952_data.device_num);

  BQ76952_PrintManufacturingStatus(bq76952_data.manufacturing_status);
  
  printf("\r\nCells (%u):\r\n",
         bq76952_data.cell_count);
  for (uint8_t i = 0; i < bq76952_data.cell_count; i++)
  {
    printf("  Cell %u: %u mV\r\n",
           i + 1,
           bq76952_data.cell_mv[i]);
  }

  printf("\r\nVoltages:\r\n");
  printf("  Stack Voltage:    %u mV\r\n",
         bq76952_data.stack_mv);
  printf("  PACK Pin Voltage: %u mV\r\n",
         bq76952_data.pack_pin_mv);
  printf("  LD Pin Voltage:   %u mV\r\n",
         bq76952_data.ld_pin_mv);

  printf("\r\nCurrent:\r\n");
  printf("  Current: %d mA\r\n",
         bq76952_data.cc2_current_ma);

  printf("\r\nTemperature:\r\n");
  printf("  Internal: %.2f C\r\n",
         bq76952_data.internal_temp_c);

  printf("\r\nSafety:\r\n");
  printf("  Status: A=0x%04X B=0x%04X C=0x%04X\r\n",
         bq76952_data.safety_status_a,
         bq76952_data.safety_status_b,
         bq76952_data.safety_status_c);
  printf("  Alert:  A=0x%04X B=0x%04X C=0x%04X\r\n",
         bq76952_data.safety_alert_a,
         bq76952_data.safety_alert_b,
         bq76952_data.safety_alert_c);

  printf("\r\nPermanent Fault:\r\n");
  printf("  Status: A=0x%04X B=0x%04X C=0x%04X D=0x%04X\r\n",
         bq76952_data.pf_status_a,
         bq76952_data.pf_status_b,
         bq76952_data.pf_status_c,
         bq76952_data.pf_status_d);
  printf("  Alert:  A=0x%04X B=0x%04X C=0x%04X D=0x%04X\r\n",
         bq76952_data.pf_alert_a,
         bq76952_data.pf_alert_b,
         bq76952_data.pf_alert_c,
         bq76952_data.pf_alert_d);
         
  printf("\r\nStatus Registers:\r\n");
  BQ76952_PrintBatteryStatus(bq76952_data.battery_status);
  printf("  Alarm Status:      0x%04X\r\n",
         bq76952_data.alarm_status);
  printf("  FET Status:        0x%04X\r\n",
         bq76952_data.fet_status);

  static const char *charge_state_str[] = {"IDLE", "CHARGING", "DISCHARGING"};
  printf("\r\nBattery State:\r\n");
  printf("  State: %s\r\n",
         charge_state_str[bq76952_data.charge_state]);
  printf("====================================\r\n");
}

static void task_1hz(void) {
  //TODO
  debug_print();
}

static void task_5hz(void) {
  BQ76952_GetSafetyAlertA(&bq76952_data.safety_alert_a);
  BQ76952_GetSafetyStatusA(&bq76952_data.safety_status_a);
  BQ76952_GetSafetyAlertB(&bq76952_data.safety_alert_b);
  BQ76952_GetSafetyStatusB(&bq76952_data.safety_status_b);
  BQ76952_GetSafetyAlertC(&bq76952_data.safety_alert_c);
  BQ76952_GetSafetyStatusC(&bq76952_data.safety_status_c);

  BQ76952_GetPFAlertA(&bq76952_data.pf_alert_a);
  BQ76952_GetPFStatusA(&bq76952_data.pf_status_a);
  BQ76952_GetPFAlertB(&bq76952_data.pf_alert_b);
  BQ76952_GetPFStatusB(&bq76952_data.pf_status_b);
  BQ76952_GetPFAlertC(&bq76952_data.pf_alert_c);
  BQ76952_GetPFStatusC(&bq76952_data.pf_status_c);
  BQ76952_GetPFAlertD(&bq76952_data.pf_alert_d);
  BQ76952_GetPFStatusD(&bq76952_data.pf_status_d);

  BQ76952_GetAlarmStatus(&bq76952_data.alarm_status);
}

static void task_10hz(void) {
  ReadCellVoltages();
  BQ76952_GetStackVoltage(&bq76952_data.stack_mv);
  BQ76952_GetPackPinVoltage(&bq76952_data.pack_pin_mv);
  BQ76952_GetLDPinVoltage(&bq76952_data.ld_pin_mv);
  BQ76952_GetCC2Current(&bq76952_data.cc2_current_ma);
  BQ76952_GetFETStatus(&bq76952_data.fet_status);
  BQ76952_GetBatteryStatus(&bq76952_data.battery_status);
  BQ76952_GetInternalTemp(&bq76952_data.internal_temp_c);

  BQ76952_UpdateChargeState();
}

void BQ76952_init() {
  memset(&bq76952_data, 0, sizeof(bq76952_data_t));
  if (!BQ76952_GetDeviceNumber(&bq76952_data.device_num)) {
    printf("Device read failed\r\n");
    return;
  }
  printf("Device Number 0x%04x\r\n", bq76952_data.device_num);

  // Factory default FET_EN is 0.
  BQ76952_SetManufacturingStatusBit(BIT_MFG_STATUS_FET_EN, true);
  BQ76952_GetManufacturingStatus(&bq76952_data.manufacturing_status);
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
  // Protections disabled for cell simulator.
  // BQ76952_ConfigureProtections()
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
  BQ76952_TestFETControl();
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
