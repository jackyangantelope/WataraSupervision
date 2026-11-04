/* SPDX-License-Identifier: MIT */
#ifndef SV_DEX_DIAG_H
#define SV_DEX_DIAG_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint64_t on_cycles,off_cycles,eligible_cycles,on_observer_us,off_observer_us,check_us,empty_us,hit_us,miss_us;
    uint32_t on_frames,off_frames,on_blocks,off_blocks;
    uint32_t other,phase,state,irq,signature,exit,budget,eligible;
    uint32_t bne,dex,x_limited,budget_limited,align_cycles;
    uint32_t timed_checks,max_check_us,empty_samples,max_empty_us;
} sv_dex_stats;
extern volatile uint32_t sv_dex_selector;
bool sv_dex_reset(void);
const char *sv_dex_name(void);
void sv_dex_frame(void);
void sv_dex_before(void *cpu,uint32_t cycles);
void sv_dex_add_outer(uint32_t us);
void sv_dex_take(sv_dex_stats *out);
uint32_t sv_dex_now(void);
bool sv_dex_code_match(void);
#ifdef __cplusplus
}
#endif
#endif
