#ifndef __CANARD_NODE_H
#define __CANARD_NODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
  uint32_t id;
  uint8_t data_len;
  uint8_t data[64U];
  uint8_t iface_id;
} CanardRxFrameObj;

#ifdef __cplusplus
}
#endif

#endif /* __CANARD_NODE_H */
