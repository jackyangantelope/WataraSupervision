

#include "sv_cpu_probe.h"
#include <string.h>
#ifndef SV_CPU_PROBE_PERIOD
#define SV_CPU_PROBE_PERIOD 1021u
#endif
static sv_cpu_probe_stats svp;
static bool svp_ready,svp_enabled,svp_active;
static uint32_t svp_frame,svp_next=SV_CPU_PROBE_PERIOD,svp_start;
static uint32_t svp_wall;
extern uint32_t sv_cpu_probe_wall_us(void);
static uint32_t svp_reads,svp_writes,svp_rom,svp_addr,svp_rc,svp_wc,svp_ac;
static bool svp_bad;
#if defined(__arm__)
static inline __attribute__((always_inline)) uint32_t svp_clock(void){
    return *(volatile uint32_t *)0xe000e018u;
}
#else
extern uint32_t sv_cpu_probe_host_clock(void);
static inline uint32_t svp_clock(void){return sv_cpu_probe_host_clock();}
#endif
static inline uint32_t svp_delta(uint32_t before,uint32_t after){return (before-after)&0xffffffu;}
void sv_cpu_probe_ready(bool ready){svp_ready=ready;}
void sv_cpu_probe_reset(void){
    memset(&svp,0,sizeof svp);svp_frame=0;svp_next=SV_CPU_PROBE_PERIOD;svp_active=false;svp_enabled=false;
}
void sv_cpu_probe_take(sv_cpu_probe_stats *out){*out=svp;memset(&svp,0,sizeof svp);}
void sv_cpu_probe_frame(void){
    svp_enabled=svp_ready&&((svp_frame/120u)&1u);svp_frame++;
    if(svp_enabled){
        svp.on_frames++;
        for(unsigned i=0;i<32;i++){uint32_t a=svp_clock(),b=svp_clock();svp.empty+=svp_delta(a,b);svp.empty_calls++;}
    }else svp.off_frames++;
}
static inline __attribute__((always_inline)) void svp_begin(void){
    svp.eligible++;
    if(svp_enabled)svp.eligible_on++;
    svp_active=svp_enabled&&!--svp_next;
    if(svp_active){
        svp_next=SV_CPU_PROBE_PERIOD;svp_reads=svp_writes=svp_rom=svp_addr=svp_rc=svp_wc=svp_ac=0;svp_bad=false;
        svp_wall=sv_cpu_probe_wall_us();svp_start=svp_clock();
    }
}
static inline __attribute__((always_inline)) void svp_end(void){
    if(!svp_active)return;
    uint32_t total=svp_delta(svp_start,svp_clock()),parts=svp_reads+svp_writes+svp_addr;
    if(svp_bad||parts>total||(uint32_t)(sv_cpu_probe_wall_us()-svp_wall)>=60000u){svp.bad++;}
    else{
        svp.samples++;svp.total+=total;svp.reads+=svp_reads;svp.writes+=svp_writes;svp.rom+=svp_rom;
        svp.addr+=svp_addr;svp.other+=total-parts;svp.read_calls+=svp_rc;svp.write_calls+=svp_wc;svp.addr_calls+=svp_ac;
        if(total>svp.max_cycles)svp.max_cycles=total;
    }
    svp_active=false;
}
static __attribute__((noinline)) uint8_t svp_timed_read(VrEmu6502 *c,uint16_t a,bool debug){
    uint32_t before=svp_clock();uint8_t v=c->readFn(a,debug);uint32_t elapsed=svp_delta(before,svp_clock());
    svp_reads+=elapsed;svp_rc++;if(a>=0x8000)svp_rom+=elapsed;return v;
}
static __attribute__((noinline)) void svp_timed_write(VrEmu6502 *c,uint16_t a,uint8_t v){
    uint32_t before=svp_clock();c->writeFn(a,v);svp_writes+=svp_delta(before,svp_clock());svp_wc++;
}
static __attribute__((noinline)) uint16_t svp_timed_mode(VrEmu6502 *c,vrEmu6502AddrModeFn fn){
    uint32_t mem=svp_reads+svp_writes,before=svp_clock();uint16_t a=fn(c);
    uint32_t elapsed=svp_delta(before,svp_clock()),nested=svp_reads+svp_writes-mem;
    if(nested>elapsed)svp_bad=true;else svp_addr+=elapsed-nested;
    svp_ac++;return a;
}

static inline __attribute__((always_inline)) uint8_t svp_read(VrEmu6502 *c,uint16_t a,bool debug){
    return svp_active?svp_timed_read(c,a,debug):c->readFn(a,debug);
}
static inline __attribute__((always_inline)) void svp_write(VrEmu6502 *c,uint16_t a,uint8_t v){
    if(svp_active)svp_timed_write(c,a,v);else c->writeFn(a,v);
}
static inline __attribute__((always_inline)) uint16_t svp_mode(VrEmu6502 *c,vrEmu6502AddrModeFn fn){
    return svp_active?svp_timed_mode(c,fn):fn(c);
}
