/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdint.h>
#include <time.h>
uint32_t r8_sv_now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint32_t)((uint64_t)t.tv_sec * 1000000 + t.tv_nsec / 1000);
}
