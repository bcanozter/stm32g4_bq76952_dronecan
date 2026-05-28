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
#define CMD_CELL13_VOLTAGE 0x2E
#define CMD_CELL14_VOLTAGE 0x30
#define CMD_CELL15_VOLTAGE 0x32
#define CMD_CELL16_VOLTAGE 0x34
#define CMD_STACK_VOLTAGE 0x36
#define CMD_PACK_PIN_VOLTAGE 0x38
#define CMD_LD_PIN_VOLTAGE 0x31

#define CMD_CC2_CURRENT 0x3A

#define CMD_INTERNAL_TEMP 0x68 // unit 0.1K
#define CMD_TS1_TEMP 0x70      // unit 0.1K
#define CMD_TS2_TEMP 0x72      // unit 0.1K
#define CMD_TS3_TEMP 0x74      // unit 0.1K

// Subcommands
#define SUB_CMD_DEVICE_NUM 0x0001
#define SUB_CMD_MANUFACTURING_STATUS 0x0057
#define SUB_CMD_FET_ENABLE 0x0022
#define SUB_CMD_RESET 0x0012

// Command Subcommand
#define SET_CFGUPDATE 0x0090
#define EXIT_CFGUPDATE 0x0092
#define FET_ENABLE 0x0022
#define _RESET 0x0012
//
#define READ 0  // Read
#define WRITE 1 // Write

typedef struct {
  uint16_t cell_mv[16];
  uint8_t device_num;
  uint16_t pack_mv;
  uint16_t ld_mv;
  float internal_temp_c;
  int16_t cc2_current_ma;
} bq76952_data_t;

bool BQ76952_GetCellVoltage(uint8_t cell, uint16_t *mv);
bool BQ76952_GetInternalTemp(float *temp);
bool BQ76952_Subcommand(uint16_t cmd, uint16_t data, uint8_t type,
                        unsigned char *rx_result);
bool BQ76952_DirectCommand(uint16_t cmd, uint16_t data, uint8_t type,
                           unsigned char *rx_result);
void BQ76952_Loop();

#ifdef __cplusplus
}
#endif

#endif /* __BQ76952_PROTOCOL_H */
