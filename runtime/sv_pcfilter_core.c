/* SPDX-License-Identifier: MIT */
#include "sv_pcfilter_reference.inc"
#include "sv_pcfilter.h"
#if R8_SV_OPFILTER
#include "sv_opfilter.h"
#ifndef SV_OPFILTER_SELECTOR
#define SV_OPFILTER_SELECTOR 1
#endif
volatile uint32_t sv_opfilter_selector __attribute__((used,section(".data.sv_opfilter_selector")))=SV_OPFILTER_SELECTOR;
static int op_mode=-1;
static sv_opfilter_stats op_stats;
bool sv_opfilter_reset(void){uint32_t s=sv_opfilter_selector;op_mode=-1;if(s!=1&&s!=2)return false;op_mode=s-1;memset(&op_stats,0,sizeof op_stats);return true;}
const char *sv_opfilter_name(void){int m=op_mode;if(m<0)m=sv_opfilter_selector==2?1:sv_opfilter_selector==1?0:-1;return m==1?"ON":m==0?"OFF":"INVALID";}
void sv_opfilter_frame(void){if(op_mode==1)op_stats.on_frames++;else op_stats.off_frames++;}
void sv_opfilter_take(sv_opfilter_stats *out){*out=op_stats;memset(&op_stats,0,sizeof op_stats);}
#endif
#ifndef SV_PCFILTER_SELECTOR
#define SV_PCFILTER_SELECTOR 1
#endif
volatile uint32_t sv_pcfilter_selector __attribute__((used,section(".data.sv_pcfilter_selector")))=SV_PCFILTER_SELECTOR;
static int pc_mode=-1;
static sv_pcfilter_stats pc_stats;
bool sv_pcfilter_reset(void){uint32_t s=sv_pcfilter_selector;pc_mode=-1;if(s!=1&&s!=2)return false;pc_mode=s-1;memset(&pc_stats,0,sizeof pc_stats);return true;}
const char *sv_pcfilter_name(void){int m=pc_mode;if(m<0)m=sv_pcfilter_selector==2?1:sv_pcfilter_selector==1?0:-1;return m==1?"ON":m==0?"OFF":"INVALID";}
void sv_pcfilter_frame(void){if(pc_mode==1)pc_stats.on_frames++;else pc_stats.off_frames++;}
void sv_pcfilter_take(sv_pcfilter_stats *out){*out=pc_stats;memset(&pc_stats,0,sizeof pc_stats);}
#if R8_SV_CPU_RAM
__attribute__((section(".time_critical.sv_pcfilter_cycles")))
#endif
__attribute__((flatten))
void vrEmu6502RunCycles(VrEmu6502 *c,uint32_t cycles){
 if(pc_mode!=1){sv_pcfilter_reference_run(c,cycles);return;}
#if R8_SV_OPFILTER
 const bool op_active=op_mode==1;
#endif
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
   uint32_t n=0;
   if(c->pc>=0x8000){
    pc_stats.rom_checks++;
#if R8_SV_OPFILTER

    // Ineligible opcodes must still execute the original tick.
    if(op_active){
     const uint8_t op=c->currentOpcode;
     if(op!=0x90u&&op!=0xd0u&&op!=0xf0u){op_stats.bypassed++;goto op_original_tick;}
     op_stats.eligible++;
    }
#endif
    n=wait_skip(c,cycles);if(!n)n=threshold_skip(c,cycles);
   if(!n){n=dex_batch_skip(c,cycles);if(n&&!had_dex){db_stats.blocks_with_batch++;had_dex=true;}}
   if(!n){n=io_batch_skip(c,cycles);if(n&&!had_io){ib_stats.blocks_with_batch++;had_io=true;}}
   if(!n){n=release_skip(c,cycles);if(n&&!had_release){release_stats.blocks_with_batch++;had_release=true;}}
   }else pc_stats.ram_bypass++;
   if(n){cycles-=n;continue;}
#if R8_SV_OPFILTER
op_original_tick:
#endif
   vrEmu6502Tick(c);cycles--;
  }
 }
}
