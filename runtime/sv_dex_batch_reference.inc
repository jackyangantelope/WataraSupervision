/* SPDX-License-Identifier: MIT */

#define vrEmu6502RunCycles sv_wait_reference_run
#include "vrEmu6502.c"
#undef vrEmu6502RunCycles
#include "sv_wait.h"
#include <string.h>
static VrEmu6502 *wait_cpu;
static vrEmu6502MemRead wait_read;
static vrEmu6502MemWrite wait_write;
static bool wait_on;
static uint32_t wait_frame;
static sv_wait_stats wait_stats;
void sv_wait_bind(void *cpu){
    wait_cpu=cpu;wait_read=wait_cpu->readFn;wait_write=wait_cpu->writeFn;
    wait_on=false;wait_frame=0;memset(&wait_stats,0,sizeof wait_stats);
}
void sv_wait_frame(int forced_mode){
    wait_stats.epoch=wait_frame/240u;
    wait_on=forced_mode<0?(wait_stats.epoch&1u)!=0:forced_mode!=0;
    wait_frame++;
    if(wait_on)wait_stats.on_frames++;else wait_stats.off_frames++;
}
void sv_wait_add_cpu(uint64_t us){
    if(wait_on)wait_stats.on_cpu_us+=us;else wait_stats.off_cpu_us+=us;
}
void sv_wait_take(sv_wait_stats *out){
    *out=wait_stats;memset(&wait_stats,0,sizeof wait_stats);
}
static inline uint32_t wait_skip(VrEmu6502 *c,uint32_t remaining){

    if(remaining<6 || c!=wait_cpu || c->model!=CPU_W65C02 || c->opcodes!=wdc65c02 ||
       c->readFn!=wait_read || c->writeFn!=wait_write || c->wai || c->stp || c->step ||
       c->zpBase || c->pc<0x8000 || (c->pc&255u)>0xfbu ||
       c->currentOpcode!=0xf0 || c->currentOpcodeAddr!=(uint16_t)(c->pc+2u) ||
       (c->flags&(FlagN|FlagZ|FlagC))!=(FlagZ|FlagC))return 0;
    wait_stats.candidates++;
    if(c->nmiPin!=IntCleared || c->intPin!=IntCleared){wait_stats.irq_reject++;return 0;}
    if(!sv_wait_readonly_match(c->pc,c->ac)){wait_stats.map_reject++;return 0;}

    uint32_t n=remaining/6u*6u;
    wait_stats.skipped+=n;wait_stats.spans++;return n;
}
#if R8_SV_CPU_RAM
__attribute__((section(".time_critical.sv_wait_cycles")))
#endif
__attribute__((flatten))
void sv_threshold_reference_run(VrEmu6502 *c,uint32_t cycles){
    if(c!=wait_cpu || !wait_on){
        if(c==wait_cpu)wait_stats.off_cycles+=cycles;
        sv_wait_reference_run(c,cycles);return;
    }
    wait_stats.on_cycles+=cycles;
    while(cycles){
        if(c->stp)return;
        if(c->step){
            uint32_t n=c->step<cycles?c->step:cycles;c->step-=n;cycles-=n;
        }else{
            uint32_t n=wait_skip(c,cycles);
            if(n){cycles-=n;continue;}
            vrEmu6502Tick(c);cycles--;
        }
    }
}

#include "sv_threshold.h"
static sv_threshold_stats threshold_stats;
static uint32_t threshold_frame;
static bool threshold_on;
void sv_threshold_reset(void){memset(&threshold_stats,0,sizeof threshold_stats);threshold_frame=0;threshold_on=false;}
void sv_threshold_frame(int force){
    threshold_stats.epoch=threshold_frame/240u;threshold_frame++;
    threshold_on=force<0?(threshold_stats.epoch&1u)!=0:force!=0;
    if(threshold_on)threshold_stats.on_frames++;else threshold_stats.off_frames++;
}
void sv_threshold_add_cpu(uint64_t us){if(threshold_on)threshold_stats.on_cpu_us+=us;else threshold_stats.off_cpu_us+=us;}
void sv_threshold_take(sv_threshold_stats *out){*out=threshold_stats;memset(&threshold_stats,0,sizeof threshold_stats);}
uint32_t sv_threshold_crc32(const uint8_t *bytes,uint32_t size){
    uint32_t crc=~0u;
    for(uint32_t i=0;i<size;i++){
        crc^=bytes[i];for(unsigned bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));
    }
    return ~crc;
}
static inline uint32_t threshold_skip(VrEmu6502 *c,uint32_t remaining){

    if(remaining<8||c->currentOpcode!=0x90||c->step||c->ac>=2||
       c!=wait_cpu||!wait_on||c->model!=CPU_W65C02||c->opcodes!=wdc65c02||
       c->readFn!=wait_read||c->writeFn!=wait_write||c->wai||c->stp||c->zpBase||
       c->pc<0x8000||(c->pc&255u)>0xf9u||c->currentOpcodeAddr!=(uint16_t)(c->pc+4u)||
       (c->flags&(FlagN|FlagZ|FlagC))!=FlagN)return 0;
    threshold_stats.candidates++;
    if(c->nmiPin!=IntCleared||c->intPin!=IntCleared){threshold_stats.irq_reject++;return 0;}
    if(!sv_threshold_readonly_match(c->pc,c->ac)){threshold_stats.map_reject++;return 0;}
    uint32_t n=remaining/8u*8u;
    threshold_stats.covered+=n;threshold_stats.spans++;return n;
}
#if R8_SV_CPU_RAM
__attribute__((section(".time_critical.sv_threshold_cycles")))
#endif
__attribute__((flatten))
void sv_dex_batch_reference_run(VrEmu6502 *c,uint32_t cycles){
    if(c!=wait_cpu||!threshold_on||!wait_on){
        if(c==wait_cpu){if(threshold_on)threshold_stats.on_cycles+=cycles;else threshold_stats.off_cycles+=cycles;}
        sv_threshold_reference_run(c,cycles);return;
    }
    threshold_stats.on_cycles+=cycles;wait_stats.on_cycles+=cycles;
    while(cycles){
        if(c->stp)return;
        if(c->step){uint32_t n=c->step<cycles?c->step:cycles;c->step-=n;cycles-=n;}
        else{
            uint32_t n=wait_skip(c,cycles);
            if(!n)n=threshold_skip(c,cycles);
            if(n){cycles-=n;continue;}
            vrEmu6502Tick(c);cycles--;
        }
    }
}
