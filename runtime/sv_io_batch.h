/* SPDX-License-Identifier: MIT */
#ifndef SV_IO_BATCH_H
#define SV_IO_BATCH_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
 uint64_t on_cycles,off_cycles,batched_cycles,on_cpu_us,off_cpu_us;
 uint32_t on_frames,off_frames,on_blocks,off_blocks,blocks_with_batch;
 uint32_t spans,candidates,irq_reject,map_reject,guard_bypass;
} sv_io_batch_stats;
extern volatile uint32_t sv_io_batch_selector;
bool sv_io_batch_reset(void);
const char *sv_io_batch_name(void);
void sv_io_batch_frame(void);
void sv_io_batch_add_cpu(uint64_t us);
void sv_io_batch_take(sv_io_batch_stats *out);

bool sv_io_batch_match(void);
#ifdef __cplusplus
}
#endif
#endif
