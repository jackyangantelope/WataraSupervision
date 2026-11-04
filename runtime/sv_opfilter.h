/* SPDX-License-Identifier: MIT */
#ifndef SV_OPFILTER_H
#define SV_OPFILTER_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t on_frames,off_frames,bypassed,eligible; } sv_opfilter_stats;
extern volatile uint32_t sv_opfilter_selector;
bool sv_opfilter_reset(void);
const char *sv_opfilter_name(void);
void sv_opfilter_frame(void);
void sv_opfilter_take(sv_opfilter_stats *out);
#ifdef __cplusplus
}
#endif
#endif
