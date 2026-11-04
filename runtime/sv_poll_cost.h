/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SV_POLL_COST_H
#define SV_POLL_COST_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint64_t io_on_us,io_off_us,other_us;
    uint32_t on_frames,off_frames,io_on_blocks,io_off_blocks,other_blocks,epoch;
} sv_poll_stats;
void sv_poll_reset(void);
void sv_poll_frame(int forced_mode);
void sv_poll_add_cpu(uint32_t us);
void sv_poll_take(sv_poll_stats *out);
bool sv_poll_code_match(uint16_t start);
bool sv_poll_input_ff(void);
#ifdef __cplusplus
}
#endif
#endif
