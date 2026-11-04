/* SPDX-License-Identifier: MIT */
#ifndef SV_RELEASE_BATCH_H
#define SV_RELEASE_BATCH_H
#include "sv_io_batch.h"
#ifdef __cplusplus
extern "C" {
#endif
extern volatile uint32_t sv_release_selector;
bool sv_release_reset(void);
const char *sv_release_name(void);
void sv_release_frame(void);
void sv_release_add_cpu(uint64_t us);
void sv_release_take(sv_io_batch_stats *out);
bool sv_release_match(uint8_t input);
#ifdef __cplusplus
}
#endif
#endif
