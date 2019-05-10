#ifndef __COUNTER_H__
#define __COUNTER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void counterInit(void);
void counterStart(void);
uint32_t counterStop(void);
void PIT2_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif
