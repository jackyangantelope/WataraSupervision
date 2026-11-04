/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SV_CPU_PROBE_CLOCK_H
#define SV_CPU_PROBE_CLOCK_H
#include <stdint.h>
#include <stdbool.h>
static inline bool sv_cpu_probe_clock_setup(volatile uint32_t *csr,
    volatile uint32_t *rvr,volatile uint32_t *cvr,uint32_t hz){
    if((*csr&3u)||hz!=252000000u)return false;
    *rvr=0xffffffu;*cvr=0;*csr=5u;
    return (*csr&7u)==5u&&*rvr==0xffffffu;
}
#endif
