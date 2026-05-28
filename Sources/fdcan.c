#include <stdint.h>
#include <string.h>

#include "fdcan.h"
#include "libcanard/canard.h"
#include "main.h"
#include "stm32g4xx_hal.h"

extern CanardInstance canard;

FDCAN_TxHeaderTypeDef TxHeader;
FDCAN_RxHeaderTypeDef RxHeader;
FDCAN_HandleTypeDef hfdcan2;

void MX_FDCAN2_Init(void) {
  hfdcan2.Instance = FDCAN2;

  hfdcan2.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan2.Init.Mode = FDCAN_MODE_NORMAL;

  hfdcan2.Init.AutoRetransmission = ENABLE;
  hfdcan2.Init.TransmitPause = DISABLE;
  hfdcan2.Init.ProtocolException = DISABLE;

  hfdcan2.Init.NominalPrescaler = 10;
  hfdcan2.Init.NominalTimeSeg1 = 13;
  hfdcan2.Init.NominalTimeSeg2 = 3;
  hfdcan2.Init.NominalSyncJumpWidth = 3;

  hfdcan2.Init.DataPrescaler = 1;
  hfdcan2.Init.DataSyncJumpWidth = 1;
  hfdcan2.Init.DataTimeSeg1 = 1;
  hfdcan2.Init.DataTimeSeg2 = 1;

  hfdcan2.Init.StdFiltersNbr = 0;
  hfdcan2.Init.ExtFiltersNbr = 1;
  hfdcan2.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  hfdcan2.Init.ClockDivider = FDCAN_CLOCK_DIV1;

  if (HAL_FDCAN_Init(&hfdcan2) != HAL_OK) {
    Error_Handler();
  }

  FDCAN_FilterTypeDef filter = {0};

  filter.IdType = FDCAN_EXTENDED_ID;
  filter.FilterIndex = 0;
  filter.FilterType = FDCAN_FILTER_MASK;
  filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;

  filter.FilterID1 = 0x00000000;
  filter.FilterID2 = 0x00000000;
  if (HAL_FDCAN_ConfigFilter(&hfdcan2, &filter) != HAL_OK) {
    Error_Handler();
  }

  HAL_FDCAN_ConfigGlobalFilter(&hfdcan2, FDCAN_ACCEPT_IN_RX_FIFO0,
                               FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_FILTER_REMOTE,
                               FDCAN_FILTER_REMOTE);
  if (HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE,
                                     0U) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_FDCAN_Start(&hfdcan2) != HAL_OK) {
    Error_Handler();
  }
}

int32_t FDCAN_Send(uint32_t id, uint8_t *data, uint8_t len) {
  TxHeader.Identifier = id;
  TxHeader.IdType = FDCAN_EXTENDED_ID;
  TxHeader.TxFrameType = FDCAN_DATA_FRAME;
  TxHeader.DataLength = len;
  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
  TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  TxHeader.MessageMarker = 0;

  if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader, data) != HAL_OK) {
    Error_Handler();
    return -1;
  }
  return 0;
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan,
                               uint32_t RxFifo0ITs) {
  if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U) {
    return;
  }

  FDCAN_RxHeaderTypeDef rxHeader;
  uint8_t rxData[8];

  if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, rxData) !=
      HAL_OK) {
    Error_Handler();
  }

  CanardCANFrame rx_frame;
  rx_frame.id = rxHeader.Identifier | CANARD_CAN_FRAME_EFF;
  rx_frame.iface_id = 0;
  rx_frame.data_len = rxHeader.DataLength;

  memcpy(rx_frame.data, rxData, rxHeader.DataLength);

  canardHandleRxFrame(&canard, &rx_frame, get_uptime_ms() * 1000ULL);
}