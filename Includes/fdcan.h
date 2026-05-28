#ifndef __FDCAN_H
#define __FDCAN_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

void MX_FDCAN2_Init(void);
int32_t FDCAN_Send(uint32_t id, uint8_t *data, uint8_t len);
void CAN_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* __FDCAN_H */