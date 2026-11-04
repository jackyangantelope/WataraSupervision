/* SPDX-License-Identifier: MIT */
#include "sv_io_batch_reference.inc"
#include "sv_io_batch.h"
#ifndef SV_IO_BATCH_SELECTOR
#define SV_IO_BATCH_SELECTOR 1
#endif
volatile uint32_t sv_io_batch_selector __attribute__((used,section(".data.sv_io_batch_selector")))=SV_IO_BATCH_SELECTOR;
static int ib_mode=-1;
static sv_io_batch_stats ib_stats;
bool sv_io_batch_reset(void){uint32_t s=sv_io_batch_selector;ib_mode=-1;if(s!=1&&s!=2)return false;ib_mode=s-1;memset(&ib_stats,0,sizeof ib_stats);return true;}
const char *sv_io_batch_name(void){int m=ib_mode;if(m<0){uint32_t s=sv_io_batch_selector;m=s==1?0:s==2?1:-1;}return m==0?"OFF":m==1?"ON":"INVALID";}
void sv_io_batch_frame(void){if(ib_mode==1)ib_stats.on_frames++;else ib_stats.off_frames++;}
void sv_io_batch_add_cpu(uint64_t us){if(ib_mode==1)ib_stats.on_cpu_us+=us;else ib_stats.off_cpu_us+=us;}
void sv_io_batch_take(sv_io_batch_stats *out){*out=ib_stats;memset(&ib_stats,0,sizeof ib_stats);}
static inline uint32_t io_batch_skip(VrEmu6502 *c,uint32_t remaining){
 if(remaining<9||c->currentOpcode!=0xf0||c->pc!=0xda57||c->step||
    c->currentOpcodeAddr!=0xda5c||c->ac!=0xff||
    (c->flags&(FlagN|FlagZ|FlagC))!=(FlagZ|FlagC)||
    c!=wait_cpu||!wait_on||!threshold_on||db_mode!=1||
    c->model!=CPU_W65C02||c->opcodes!=wdc65c02||c->readFn!=wait_read||c->writeFn!=wait_write||c->wai||c->stp||c->zpBase)return 0;
 ib_stats.candidates++;
 if(c->intPin!=IntCleared||c->nmiPin!=IntCleared){ib_stats.irq_reject++;return 0;}
 if(!sv_io_batch_match()){ib_stats.map_reject++;return 0;}

 uint32_t n=remaining/9u;ib_stats.batched_cycles+=(uint64_t)n*9u;ib_stats.spans++;return n*9u;
}
#if R8_SV_CPU_RAM
__attribute__((section(".time_critical.sv_io_batch_cycles")))
#endif
__attribute__((flatten))
void sv_release_reference_run(VrEmu6502 *c,uint32_t cycles){
 if(c==wait_cpu){if(ib_mode==1){ib_stats.on_blocks++;ib_stats.on_cycles+=cycles;}else{ib_stats.off_blocks++;ib_stats.off_cycles+=cycles;}}
 if(ib_mode!=1||c!=wait_cpu||!wait_on||!threshold_on||db_mode!=1){
  if(ib_mode==1&&c==wait_cpu)ib_stats.guard_bypass++;
  sv_io_batch_reference_run(c,cycles);return;
 }
 db_stats.on_blocks++;db_stats.on_cycles+=cycles;
 threshold_stats.on_cycles+=cycles;wait_stats.on_cycles+=cycles;bool had_dex=false,had_io=false;
 while(cycles){
  if(c->stp)return;
  if(c->step){uint32_t n=c->step<cycles?c->step:cycles;c->step-=n;cycles-=n;}
  else{
   uint32_t n=wait_skip(c,cycles);if(!n)n=threshold_skip(c,cycles);
   if(!n){n=dex_batch_skip(c,cycles);if(n&&!had_dex){db_stats.blocks_with_batch++;had_dex=true;}}
   if(!n){n=io_batch_skip(c,cycles);if(n&&!had_io){ib_stats.blocks_with_batch++;had_io=true;}}
   if(n){cycles-=n;continue;}
   vrEmu6502Tick(c);cycles--;
  }
 }
}
