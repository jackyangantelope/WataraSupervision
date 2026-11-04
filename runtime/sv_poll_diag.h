/* SPDX-License-Identifier: MIT */
#ifndef SV_POLL_DIAG_H
#define SV_POLL_DIAG_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint16_t pc,addr;uint8_t op,step,a,flags,input,irq,nmi,reason;} sv_poll_state;
typedef struct {
 uint64_t check_us,hit_us,miss_us,outer_us,empty_us;
 uint32_t blocks,watched,eligible,other,state,phase,irq,signature,key,lda,cmp,beq;
 uint32_t max_us,empty_samples,reads,ff,nonff,mismatch,frames;
 sv_poll_state first,last;
} sv_poll_diag_stats;
extern volatile uint32_t sv_poll_diag_selector;
bool sv_poll_diag_reset(void);
bool sv_poll_diag_active(void);
const char *sv_poll_diag_name(void);
void sv_poll_diag_frame(void);
void sv_poll_diag_before(void *cpu);
void sv_poll_diag_io(uint8_t value);
void sv_poll_diag_take(sv_poll_diag_stats *out);
uint32_t sv_poll_diag_now(void);
bool sv_poll_diag_code_match(void);
uint8_t sv_poll_diag_input(void);
#ifdef __cplusplus
}
#endif
#endif
