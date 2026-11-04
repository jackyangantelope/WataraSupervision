/* SPDX-License-Identifier: MIT */
#include "sv_release_reference.inc"
#include "sv_release_batch.h"
#ifndef SV_RELEASE_SELECTOR
#define SV_RELEASE_SELECTOR 1
#endif
volatile uint32_t sv_release_selector __attribute__((used,section(".data.sv_release_selector")))=SV_RELEASE_SELECTOR;
static int release_mode=-1;
static sv_io_batch_stats release_stats;
bool sv_release_reset(void){uint32_t s=sv_release_selector;release_mode=-1;if(s!=1&&s!=2)return false;release_mode=s-1;memset(&release_stats,0,sizeof release_stats);return true;}
const char *sv_release_name(void){int m=release_mode;if(m<0){uint32_t s=sv_release_selector;m=s==1?0:s==2?1:-1;}return m==1?"ON":m==0?"OFF":"INVALID";}
void sv_release_frame(void){if(release_mode==1)release_stats.on_frames++;else release_stats.off_frames++;}
void sv_release_add_cpu(uint64_t us){if(release_mode==1)release_stats.on_cpu_us+=us;else release_stats.off_cpu_us+=us;}
void sv_release_take(sv_io_batch_stats *out){*out=release_stats;memset(&release_stats,0,sizeof release_stats);}
static inline uint32_t release_skip(VrEmu6502 *c,uint32_t remaining){
 if(remaining<9||c->currentOpcode!=0xd0||c->pc!=0xda5f||c->step||c->currentOpcodeAddr!=0xda64||c->ac==255||
    (c->flags&(FlagN|FlagZ|FlagC))!=((uint8_t)(c->ac+1u)&FlagN)||
    c!=wait_cpu||!wait_on||!threshold_on||db_mode!=1||ib_mode!=1||
    c->model!=CPU_W65C02||c->opcodes!=wdc65c02||c->readFn!=wait_read||c->writeFn!=wait_write||c->wai||c->stp||c->zpBase)return 0;
 release_stats.candidates++;
 if(c->intPin!=IntCleared||c->nmiPin!=IntCleared){release_stats.irq_reject++;return 0;}
 if(!sv_release_match(c->ac)){release_stats.map_reject++;return 0;}

 uint32_t n=remaining/9u;release_stats.batched_cycles+=(uint64_t)n*9u;release_stats.spans++;return n*9u;
}
#if R8_SV_CPU_RAM
__attribute__((section(".time_critical.sv_release_cycles")))
#endif
__attribute__((flatten))
void sv_pcfilter_reference_run(VrEmu6502 *c,uint32_t cycles){
 if(c==wait_cpu){if(release_mode==1){release_stats.on_blocks++;release_stats.on_cycles+=cycles;}else{release_stats.off_blocks++;release_stats.off_cycles+=cycles;}}
 if(release_mode!=1||c!=wait_cpu||!wait_on||!threshold_on||db_mode!=1||ib_mode!=1){
  if(release_mode==1&&c==wait_cpu)release_stats.guard_bypass++;
  sv_release_reference_run(c,cycles);return;
 }
 ib_stats.on_blocks++;ib_stats.on_cycles+=cycles;db_stats.on_blocks++;db_stats.on_cycles+=cycles;
 threshold_stats.on_cycles+=cycles;wait_stats.on_cycles+=cycles;bool had_dex=false,had_io=false,had_release=false;
 while(cycles){
  if(c->stp)return;
  if(c->step){uint32_t n=c->step<cycles?c->step:cycles;c->step-=n;cycles-=n;}
  else{
   uint32_t n=wait_skip(c,cycles);if(!n)n=threshold_skip(c,cycles);
   if(!n){n=dex_batch_skip(c,cycles);if(n&&!had_dex){db_stats.blocks_with_batch++;had_dex=true;}}
   if(!n){n=io_batch_skip(c,cycles);if(n&&!had_io){ib_stats.blocks_with_batch++;had_io=true;}}
   if(!n){n=release_skip(c,cycles);if(n&&!had_release){release_stats.blocks_with_batch++;had_release=true;}}
   if(n){cycles-=n;continue;}
   vrEmu6502Tick(c);cycles--;
  }
 }
}
