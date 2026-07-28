#ifndef __BQ76952_PROTOCOL_H
#define __BQ76952_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define TI_BQ_I2C_ADDRESS (0x08 << 1)
// Direct Commands
#define CMD_ALARM_ENABLE 0x66
#define CMD_ALARM_STATUS 0x62
#define CMD_ALARM_RAW_STATUS 0x64
#define CMD_BATTERY_STATUS 0x12

// mV
#define CMD_CELL1_VOLTAGE 0x14
#define CMD_CELL2_VOLTAGE 0x16
#define CMD_CELL3_VOLTAGE 0x18
#define CMD_CELL4_VOLTAGE 0x1A
#define CMD_CELL5_VOLTAGE 0x1C
#define CMD_CELL6_VOLTAGE 0x1E
#define CMD_CELL7_VOLTAGE 0x20
#define CMD_CELL8_VOLTAGE 0x22
#define CMD_CELL9_VOLTAGE 0x24
#define CMD_CELL10_VOLTAGE 0x26
#define CMD_CELL11_VOLTAGE 0x28
#define CMD_CELL12_VOLTAGE 0x2A
#define CMD_CELL13_VOLTAGE 0x2C
#define CMD_CELL14_VOLTAGE 0x2E
#define CMD_CELL15_VOLTAGE 0x30
#define CMD_CELL16_VOLTAGE 0x32
#define CMD_STACK_VOLTAGE 0x34
#define CMD_PACK_PIN_VOLTAGE 0x36
#define CMD_LD_PIN_VOLTAGE 0x38

#define CMD_CC2_CURRENT 0x3A

#define CMD_INTERNAL_TEMP 0x68 // unit 0.1K
#define CMD_TS1_TEMP 0x70      // unit 0.1K
#define CMD_TS2_TEMP 0x72      // unit 0.1K
#define CMD_TS3_TEMP 0x74      // unit 0.1K

#define CMD_FET_STATUS 0x7F
#define CMD_SAFETY_ALERT_A 0x02
#define CMD_SAFETY_STATUS_A 0x03
#define CMD_SAFETY_ALERT_B 0x04
#define CMD_SAFETY_STATUS_B 0x05
#define CMD_SAFETY_ALERT_C 0x06
#define CMD_SAFETY_STATUS_C 0x07
#define CMD_PF_ALERT_A 0x0A
#define CMD_PF_STATUS_A 0x0B
#define CMD_PF_ALERT_B 0x0C
#define CMD_PF_STATUS_B 0x0D
#define CMD_PF_ALERT_C 0x0E
#define CMD_PF_STATUS_C 0x0F
#define CMD_PF_ALERT_D 0x10
#define CMD_PF_STATUS_D 0x11

// Subcommands
#define SUB_CMD_DEVICE_NUM 0x0001
#define SUB_CMD_MANUFACTURING_STATUS 0x0057
#define SUB_CMD_FET_ENABLE 0x0022
#define SUB_CMD_RESET 0x0012
#define SUB_CMD_FET_CONTROL 0x0097

// Command Subcommand
#define SET_CFGUPDATE 0x0090
#define EXIT_CFGUPDATE 0x0092
#define FET_ENABLE 0x0022
#define ALL_FETS_OFF 0x0095
#define ALL_FETS_ON 0x0096
#define SLEEP_ENABLE 0x0099
#define _RESET 0x0012

// 5.2.3.2 FET Control
#define PDSGTEST 0x001C
#define PCHGTEST 0x001E
#define CHGTEST 0x001F
#define DSGTEST 0x0020
#define PF_ENABLE 0x0024

// Rest of the FET Control Register bits are reserved.
#define BIT_FET_CONTROL_DSG_OFF (1U << 0)
#define BIT_FET_CONTROL_PDSG_OFF (1U << 1)
#define BIT_FET_CONTROL_CHG_OFF (1U << 2)
#define BIT_FET_CONTROL_PCHG_OFF (1U << 3)

// Manufacturing Status Register (12.5.5).
#define BIT_MFG_STATUS_PCHG_TEST (1U << 0)
#define BIT_MFG_STATUS_CHG_TEST (1U << 1)
#define BIT_MFG_STATUS_DSG_TEST (1U << 2)
#define BIT_MFG_STATUS_FET_EN (1U << 4)
#define BIT_MFG_STATUS_PDSG_TEST (1U << 5)
#define BIT_MFG_STATUS_PF_EN (1U << 6)
#define BIT_MFG_STATUS_OTPW_EN (1U << 7)

// Settings
#define REG_VCELL_MODE 0x9304
#define REG_ENABLED_PROTECTIONS_A 0x9261
#define REG_ENABLED_PROTECTIONS_B 0x9262
#define REG_ENABLED_PROTECTIONS_C 0x9263

// 16S cell simulator -> all 16 cell positions active.
// Table 13-16 in the technical manual explains the register fields.
#define VCELL_MODE_16S 0xFFFF

// Enabled Protections A: factory default
#define ENABLED_PROTECTIONS_A_VALUE 0x8C
#define ENABLED_PROTECTIONS_B_VALUE 0x77

//
#define READ 0  // Read
#define WRITE 1 // Write

// https://www.ti.com/lit/ug/sluuby2b/sluuby2b.pdf
// 12.2.16 Battery Status Register
#define BIT_CFGUPDATE (1U << 0)
#define BIT_PCHG_MODE (1U << 1)
#define BIT_SLEEP_EN (1U << 2)
#define BIT_POR (1U << 3)
#define BIT_WD (1U << 4)
#define BIT_COW_CHK (1U << 5)
#define BIT_OTPW (1U << 6)
#define BIT_OTPB (1U << 7)

#define BIT_SEC0 (1U << 8)
#define BIT_SEC1 (1U << 9)
#define BIT_FUSE (1U << 10)
#define BIT_SS (1U << 11)
#define BIT_PF (1U << 12)
#define BIT_SD_CMD (1U << 13)
#define BIT_RSVD_14 (1U << 14)
#define BIT_SLEEP (1U << 15)

// end of api macros

#define BQ_CHARGE_CURRENT_THRESHOLD_MA 200
#define BQ_DISCHARGE_CURRENT_THRESHOLD_MA 200
#define BQ_CHARGE_STATE_HYSTERESIS_MA 100
typedef enum {
  BQ_CHARGE_STATE_IDLE = 0,
  BQ_CHARGE_STATE_CHARGING,
  BQ_CHARGE_STATE_DISCHARGING,
} bq76952_charge_state_t;



typedef struct {
  uint16_t cell_mv[16];
  uint8_t  cell_count;

  uint16_t device_num;

  uint16_t battery_status;

  uint16_t safety_alert_a;
  uint16_t safety_alert_b;
  uint16_t safety_alert_c;
  uint16_t safety_status_a;
  uint16_t safety_status_b;
  uint16_t safety_status_c;

  uint16_t pf_alert_a;
  uint16_t pf_alert_b;
  uint16_t pf_alert_c;
  uint16_t pf_alert_d;
  uint16_t pf_status_a;
  uint16_t pf_status_b;
  uint16_t pf_status_c;
  uint16_t pf_status_d;

  uint16_t alarm_status;
  uint16_t fet_status;

  uint16_t stack_mv;
  uint16_t pack_pin_mv;
  uint16_t ld_pin_mv;

  float internal_temp_c;
  int16_t cc2_current_ma;

  uint16_t manufacturing_status;
  bq76952_charge_state_t charge_state;
} bq76952_data_t;

bool BQ76952_ExitConfigUpdateMode();
bool BQ76952_EnterConfigUpdateMode();
bool BQ76952_GetDeviceNumber(uint16_t *device_num);
bool BQ76952_GetBatteryStatus(uint16_t *battery_status);
bool BQ76952_GetCellVoltage(uint8_t cell, uint16_t *mv);
bool BQ76952_GetInternalTemp(float *temp);
bool BQ76952_GetStackVoltage(uint16_t *mv);
bool BQ76952_GetPackPinVoltage(uint16_t *mv);
bool BQ76952_GetLDPinVoltage(uint16_t *mv);
bool BQ76952_GetCC2Current(int16_t *current_ma);
bool BQ76952_GetFETStatus(uint16_t *status);

bool BQ76952_GetSafetyAlertA(uint16_t *status);
bool BQ76952_GetSafetyStatusA(uint16_t *status);
bool BQ76952_GetSafetyAlertB(uint16_t *status);
bool BQ76952_GetSafetyStatusB(uint16_t *status);
bool BQ76952_GetSafetyAlertC(uint16_t *status);
bool BQ76952_GetSafetyStatusC(uint16_t *status);

bool BQ76952_GetPFAlertA(uint16_t *status);
bool BQ76952_GetPFStatusA(uint16_t *status);
bool BQ76952_GetPFAlertB(uint16_t *status);
bool BQ76952_GetPFStatusB(uint16_t *status);
bool BQ76952_GetPFAlertC(uint16_t *status);
bool BQ76952_GetPFStatusC(uint16_t *status);
bool BQ76952_GetPFAlertD(uint16_t *status);
bool BQ76952_GetPFStatusD(uint16_t *status);

bool BQ76952_GetAlarmStatus(uint16_t *status);

bool BQ76952_AllFETsOff(void);
bool BQ76952_AllFETsOn(void);
bool BQ76952_SetFETControl(uint8_t fet_control_bits);
bool BQ76952_TestFETControl(void);
bool BQ76952_GetManufacturingStatus(uint16_t *status);
bool BQ76952_SetManufacturingStatusBit(uint16_t bit_mask, bool desired_state);
void BQ76952_PrintManufacturingStatus(uint16_t status);
void BQ76952_PrintBatteryStatus(uint16_t status);


bool BQ76952_DataRAM_Read(uint16_t reg, uint8_t *data, uint8_t len);
bool BQ76952_DataRAM_Write(uint16_t reg, const uint8_t *data, uint8_t len);
bool BQ76952_ConfigureProtections(void);
uint8_t BQ76952_Checksum(const uint8_t *data, uint8_t len);
uint8_t BQ76952_CRC8(const uint8_t *data, uint8_t len);
bool BQ76952_Subcommand(uint16_t cmd, uint16_t data, uint8_t type,
                        unsigned char *rx_result, uint8_t rx_len);
bool BQ76952_DirectCommand(uint16_t cmd, uint16_t data, uint8_t type,
                           unsigned char *rx_result, uint8_t len);
void BQ76952_init();
void BQ76952_loop();

//
void ReadCellVoltages(void);
bq76952_charge_state_t BQ76952_DetectChargeState(int16_t current_ma,
  bq76952_charge_state_t prev_state);
void BQ76952_UpdateChargeState(void);


#ifdef __cplusplus
}
#endif

#endif /* __BQ76952_PROTOCOL_H */
