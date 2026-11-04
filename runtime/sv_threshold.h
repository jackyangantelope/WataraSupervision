/* SPDX-License-Identifier: MIT */
#ifndef SV_THRESHOLD_H
#define SV_THRESHOLD_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint64_t on_cycles,off_cycles,covered,on_cpu_us,off_cpu_us;
    uint32_t on_frames,off_frames,spans,candidates,irq_reject,map_reject,epoch;
} sv_threshold_stats;
void sv_threshold_reset(void);
void sv_threshold_frame(int force);
void sv_threshold_add_cpu(uint64_t us);
void sv_threshold_take(sv_threshold_stats *out);
bool sv_threshold_readonly_match(uint16_t pc,uint8_t a);
uint32_t sv_threshold_crc32(const uint8_t *bytes,uint32_t size);
uint32_t r8_sv_rom_crc32(void);
#ifdef __cplusplus
}
#endif
#endif
