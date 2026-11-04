/* SPDX-License-Identifier: MIT */
#include "sv_dex_batch_reference.inc"
#include "sv_dex_batch.h"
#ifndef SV_DEX_BATCH_SELECTOR
#define SV_DEX_BATCH_SELECTOR 1
#endif
volatile uint32_t sv_dex_batch_selector __attribute__((used,section(".data.sv_dex_batch_selector")))=SV_DEX_BATCH_SELECTOR;
static int db_mode=-1;
static sv_dex_batch_stats db_stats;
bool sv_dex_batch_reset(void){
    uint32_t s=sv_dex_batch_selector;db_mode=-1;if(s!=1&&s!=2)return false;
    db_mode=(int)s-1;memset(&db_stats,0,sizeof db_stats);return true;
}
const char *sv_dex_batch_name(void){int m=db_mode;if(m<0){uint32_t s=sv_dex_batch_selector;m=s==1?0:s==2?1:-1;}return m==0?"OFF":m==1?"ON":"INVALID";}
void sv_dex_batch_frame(void){if(db_mode==1)db_stats.on_frames++;else db_stats.off_frames++;}
void sv_dex_batch_add_cpu(uint64_t us){if(db_mode==1)db_stats.on_cpu_us+=us;else db_stats.off_cpu_us+=us;}
void sv_dex_batch_take(sv_dex_batch_stats *out){*out=db_stats;memset(&db_stats,0,sizeof db_stats);}
static inline uint32_t dex_batch_skip(VrEmu6502 *c,uint32_t remaining){
    if(remaining<5||c->currentOpcode!=0xd0||c->pc!=0xe01f||c->step||c->ix<=1||
       c!=wait_cpu||!wait_on||!threshold_on||c->model!=CPU_W65C02||c->opcodes!=wdc65c02||
       c->readFn!=wait_read||c->writeFn!=wait_write||c->wai||c->stp||c->zpBase||
       c->currentOpcodeAddr!=0xe020||
       (c->flags&(FlagN|FlagZ))!=(c->ix&0x80?FlagN:0))return 0;
    db_stats.candidates++;
    if(c->nmiPin!=IntCleared||c->intPin!=IntCleared){db_stats.irq_reject++;return 0;}
    if(!sv_dex_batch_code_match()){db_stats.map_reject++;return 0;}
    uint32_t n=remaining/5u,limit=(uint32_t)c->ix-1u;if(n>limit)n=limit;

    c->ix=(uint8_t)(c->ix-n);setNZ(c,c->ix);
    db_stats.batched_cycles+=(uint64_t)n*5u;db_stats.spans++;return n*5u;
}
#if R8_SV_CPU_RAM
__attribute__((section(".time_critical.sv_dex_batch_cycles")))
#endif
__attribute__((flatten))
void sv_io_batch_reference_run(VrEmu6502 *c,uint32_t cycles){
    if(c==wait_cpu){
        if(db_mode==1){db_stats.on_blocks++;db_stats.on_cycles+=cycles;}
        else{db_stats.off_blocks++;db_stats.off_cycles+=cycles;}
    }
    if(db_mode!=1||c!=wait_cpu||!wait_on||!threshold_on){
        if(db_mode==1&&c==wait_cpu)db_stats.guard_bypass++;
        sv_dex_batch_reference_run(c,cycles);return;
    }
    threshold_stats.on_cycles+=cycles;wait_stats.on_cycles+=cycles;bool had_batch=false;
    while(cycles){
        if(c->stp)return;
        if(c->step){uint32_t n=c->step<cycles?c->step:cycles;c->step-=n;cycles-=n;}
        else{
            uint32_t n=wait_skip(c,cycles);
            if(!n)n=threshold_skip(c,cycles);
            if(!n){n=dex_batch_skip(c,cycles);if(n&&!had_batch){db_stats.blocks_with_batch++;had_batch=true;}}
            if(n){cycles-=n;continue;}
            vrEmu6502Tick(c);cycles--;
        }
    }
}
