/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SV_CPU_PROBE_H
#define SV_CPU_PROBE_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint64_t total,reads,writes,rom,addr,other,empty;
    uint32_t samples,read_calls,write_calls,addr_calls,eligible,eligible_on,bad;
    uint32_t on_frames,off_frames,empty_calls,max_cycles;
} sv_cpu_probe_stats;
void sv_cpu_probe_ready(bool ready);
void sv_cpu_probe_reset(void);
void sv_cpu_probe_frame(void);
void sv_cpu_probe_take(sv_cpu_probe_stats *out);
#ifdef __cplusplus
}
#endif
#endif
