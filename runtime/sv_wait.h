/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SV_WAIT_H
#define SV_WAIT_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint64_t on_cycles,off_cycles,skipped,on_cpu_us,off_cpu_us;
    uint32_t on_frames,off_frames,spans,candidates,irq_reject,map_reject,epoch;
} sv_wait_stats;

void sv_wait_bind(void *cpu);
void sv_wait_frame(int forced_mode);
void sv_wait_add_cpu(uint64_t us);
void sv_wait_take(sv_wait_stats *out);
bool sv_wait_readonly_match(uint16_t pc,uint8_t a);
#ifdef __cplusplus
}
#endif
#endif
