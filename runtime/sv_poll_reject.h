/* SPDX-License-Identifier: MIT */
#ifndef SV_POLL_REJECT_H
#define SV_POLL_REJECT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { SV_REJECT_PERIOD=1021, SV_REJECT_START=0xda57, SV_REJECT_END=0xda5d };
typedef struct {
    uint64_t us;
    uint32_t calls,samples,on,off,other,state,irq,accum,phase,bounds,sig,key,pass,max_us;
    uint16_t pc,wpc;
    uint8_t op,a,p,k;
} sv_reject_stats;
void sv_poll_reject_reset(void);
void sv_poll_reject_before(void *cpu);
void sv_poll_reject_take(sv_reject_stats *out);

uint8_t sv_poll_input_value(void);
uint32_t sv_poll_reject_now(void);
#ifdef __cplusplus
}
#endif
#endif
