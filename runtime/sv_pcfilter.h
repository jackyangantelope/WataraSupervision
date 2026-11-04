/* SPDX-License-Identifier: MIT */
#ifndef SV_PCFILTER_H
#define SV_PCFILTER_H
#include "sv_release_batch.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint32_t on_frames,off_frames,ram_bypass,rom_checks;} sv_pcfilter_stats;
extern volatile uint32_t sv_pcfilter_selector;
bool sv_pcfilter_reset(void);
const char *sv_pcfilter_name(void);
void sv_pcfilter_frame(void);
void sv_pcfilter_take(sv_pcfilter_stats *out);
#ifdef __cplusplus
}
#endif
#endif
