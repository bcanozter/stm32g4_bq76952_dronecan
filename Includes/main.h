#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

void Error_Handler(void);

uint32_t get_uptime_ms();
uint32_t get_uptime_sec();

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
