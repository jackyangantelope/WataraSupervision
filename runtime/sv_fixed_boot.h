/* SPDX-License-Identifier: MIT */
#ifndef SV_FIXED_BOOT_H
#define SV_FIXED_BOOT_H
#include "retro8.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { SV_FIXED_INITIAL=1,SV_FIXED_CHANGE=2,SV_FIXED_MARK=3,SV_FIXED_RING=16,SV_FIXED_LINE=256 };
typedef struct {
    uint32_t seq,frame;
    uint64_t hash,elapsed_us;
    r8_input input;
    uint8_t mapped,kind;
} sv_fixed_event;
typedef struct {
    uint32_t frames,initial,changes,marks,queued,dropped,pending,total_frames,total_events,total_dropped;
    uint64_t hash;
} sv_fixed_stats;
extern volatile uint32_t sv_fixed_selector;
bool sv_fixed_reset(void);
int sv_fixed_mode(void);
const char *sv_fixed_name(void);
void sv_fixed_begin(uint64_t now);
void sv_fixed_capture(const r8_input *input,uint8_t mapped);
void sv_fixed_frame(void);
void sv_fixed_completed(uint64_t now);
bool sv_fixed_pop(sv_fixed_event *out);
void sv_fixed_take(sv_fixed_stats *out);
int sv_fixed_format(char *out,size_t capacity,const sv_fixed_event *e);
#ifdef __cplusplus
}
#endif
#endif
