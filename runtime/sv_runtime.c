/* SPDX-License-Identifier: BSD-3-Clause */
#include "retro8.h"
#include "input_map.h"
#include "supervision.h"
#include "controls.h"
#include "memorymap.h"
#include "gpu.h"
#include "timer.h"
#include "sound.h"
#include "sv_cpu.h"
#include "vrEmu6502.h"
#if R8_SV_PCFILTER
#include "sv_pcfilter.h"
#if R8_SV_OPFILTER
#include "sv_opfilter.h"
#endif
#endif
#if R8_SV_RELEASE_BATCH
#include "sv_release_batch.h"
#endif
#if R8_SV_RELEASE_BATCH
bool sv_release_match(uint8_t input){
 const uint8_t *b=memorymap_getUpperRomBank();
 static const uint8_t sig[]={0xad,0x20,0x20,0xc9,0xff,0xd0,0xf9};
 return b&&input!=255&&controls_read()==input&&!memcmp(b+0x1a5f,sig,sizeof sig);
}
#endif
#if R8_SV_IO_BATCH
#include "sv_io_batch.h"
bool sv_io_batch_match(void){
 const uint8_t *b=memorymap_getUpperRomBank();
 static const uint8_t sig[]={0xad,0x20,0x20,0xc9,0xff,0xf0,0xf9};
 return b&&controls_read()==0xff&&!memcmp(b+0x1a57,sig,sizeof sig);
}
#endif
#if R8_SV_POLL_DIAG
#include "sv_poll_diag.h"
uint32_t sv_poll_diag_now(void){return r8_sv_now();}
uint8_t sv_poll_diag_input(void){return controls_read();}
bool sv_poll_diag_code_match(void){
 const uint8_t *b=memorymap_getUpperRomBank();
 static const uint8_t sig[]={0xad,0x20,0x20,0xc9,0xff,0xf0,0xf9};
 return b&&!memcmp(b+0x1a57,sig,sizeof sig);
}
#endif
#if R8_SV_DEX_BATCH
#include "sv_dex_batch.h"
bool sv_dex_batch_code_match(void){
    const uint8_t *bank=memorymap_getUpperRomBank();
    return bank&&bank[0x201f]==0xca&&bank[0x2020]==0xd0&&bank[0x2021]==0xfd;
}
#endif
#if R8_SV_DEX_DIAG
#include "sv_dex_diag.h"
uint32_t sv_dex_now(void){return r8_sv_now();}
bool sv_dex_code_match(void){
    const uint8_t *bank=memorymap_getUpperRomBank();
    return bank&&bank[0x201f]==0xca&&bank[0x2020]==0xd0&&bank[0x2021]==0xfd;
}
#endif
#if R8_SV_FIXED_BOOT
#include "sv_fixed_boot.h"
#endif
#if R8_SV_THRESHOLD_AB
#include "sv_threshold.h"
#ifndef SV_THRESHOLD_FORCE_MODE
#define SV_THRESHOLD_FORCE_MODE -1
#endif
static uint32_t sv_rom_crc;
uint32_t r8_sv_rom_crc32(void){return sv_rom_crc;}
bool sv_threshold_readonly_match(uint16_t pc,uint8_t a){
    static const uint8_t code[]={0xa5,0x04,0xc9,0x02,0x90,0xfa};
    if(pc<0x8000||(pc&255u)>0xf9u)return false;
    const uint8_t *bank=pc<0xc000?memorymap_getLowerRomBank():memorymap_getUpperRomBank();
    const uint8_t *ram=memorymap_getLowerRamPointer();
    if(!bank||!ram||ram[4]!=a)return false;
    const uint8_t *p=bank+(pc&0x3fffu);
    for(unsigned i=0;i<sizeof code;i++)if(p[i]!=code[i])return false;
    return true;
}
#endif
#if R8_SV_POLL_REJECT_DIAG
#include "sv_poll_reject.h"
uint8_t sv_poll_input_value(void){return controls_read();}
uint32_t sv_poll_reject_now(void){return r8_sv_now();}
#endif
#if R8_SV_POLL_COST_AB
#include "sv_poll_cost.h"
#ifndef SV_POLL_FORCE_MODE
#define SV_POLL_FORCE_MODE -1
#endif
bool sv_poll_code_match(uint16_t start){
    static const uint8_t code[]={0xad,0x20,0x20,0xc9,0xff,0xf0,0xf9};
    if(start<0x8000||(start&255u)>0xf8u)return false;
    const uint8_t *bank=start<0xc000?memorymap_getLowerRomBank():memorymap_getUpperRomBank();
    if(!bank)return false;
    const uint8_t *p=bank+(start&0x3fffu);
    for(unsigned i=0;i<sizeof code;i++)if(p[i]!=code[i])return false;
    return true;
}
bool sv_poll_input_ff(void){return controls_read()==0xff;}
#endif
#if R8_SV_WAIT_AB
#include "sv_wait.h"
#ifndef SV_WAIT_FORCE_MODE
#define SV_WAIT_FORCE_MODE -1
#endif
bool sv_wait_readonly_match(uint16_t pc,uint8_t a){
    if(pc<0x8000 || (pc&255u)>0xfbu)return false;
    const uint8_t *bank=pc<0xc000?memorymap_getLowerRomBank():memorymap_getUpperRomBank();
    const uint8_t *ram=memorymap_getLowerRamPointer();
    if(!bank||!ram)return false;
    const uint8_t *p=bank+(pc&0x3fffu);
    return p[0]==0xc5 && p[2]==0xf0 && p[3]==0xfc && ram[p[1]]==a;
}
#endif
#if R8_SV_CPU_PROBE
#include "sv_cpu_probe.h"
#endif
#include <string.h>
static VrEmu6502 *cpu;
static uint16_t *pixels;
#if R8_SV_SRAM_PIXELS
static uint16_t fast_pixels[160*160];
#endif
static r8_audio_fn output;
static r8_service_fn service;
static uint32_t fraction;
static const char *error="";
#if R8_SV_ROM_CACHE
uint32_t r8_sv_cache_fills(void){return memorymap_cache_fills();}
#endif
#if R8_SV_SPLIT_PROFILE
static r8_sv_stats diag;
static uint32_t diag_frame;
void r8_sv_get(r8_sv_stats *out){*out=diag;}
void r8_sv_reset(void){memset(&diag,0,sizeof diag);}
#endif
static uint8_t read_cpu(uint16_t a,bool dbg){(void)dbg;uint8_t v=Rd6502(a);
#if R8_SV_POLL_DIAG
 if(a==0x2020)sv_poll_diag_io(v);
#endif
 return v;
}
void m6502_set_irq_line(BOOL set){if(cpu)*vrEmu6502Int(cpu)=set?IntRequested:IntCleared;}
bool r8_init(void *bulk,size_t n,r8_audio_fn a,r8_service_fn s){
    if(n<160*160*2)return false;pixels=bulk;
#if R8_SV_SRAM_PIXELS
    pixels=fast_pixels;
#endif
    output=a;service=s;gpu_init();memorymap_init();return true;
}
bool r8_load(const uint8_t *game,size_t n,const uint8_t *bios,size_t bn,const char *name){
    (void)bios;(void)bn;(void)name;error="";
    if(!game||n<16384||n>524288||(n&16383)){error="SV ROM: 16K banks, max 512K";return false;}
#if R8_SV_FIXED_BOOT
    if(!sv_fixed_reset()){error="SV fixed selector invalid";return false;}
#endif
#if R8_SV_DEX_DIAG
    if(!sv_dex_reset()){error="SV DEX selector invalid";return false;}
#endif
#if R8_SV_DEX_BATCH
#if R8_SV_POLL_DIAG
    if(!sv_poll_diag_reset()){error="SV poll selector invalid";return false;}
#endif
#if R8_SV_IO_BATCH
#if R8_SV_RELEASE_BATCH
#if R8_SV_PCFILTER
    if(!sv_pcfilter_reset()){error="SV PC filter selector invalid";return false;}
#if R8_SV_OPFILTER
    if(!sv_opfilter_reset()){error="SV opcode filter selector invalid";return false;}
#endif
#endif
    if(!sv_release_reset()){error="SV release selector invalid";return false;}
#endif
    if(!sv_io_batch_reset()){error="SV IO batch selector invalid";return false;}
#endif
    if(!sv_dex_batch_reset()){error="SV DEX batch selector invalid";return false;}
#endif
    if(!memorymap_load(game,n)){error="Invalid SV ROM";return false;}
    if(cpu){vrEmu6502Destroy(cpu);cpu=0;}
    controls_reset();gpu_reset();memorymap_reset();sound_reset();timer_reset();
    cpu=vrEmu6502New(CPU_W65C02,read_cpu,Wr6502);
    if(!cpu){error="CPU allocation failed";return false;}
#if R8_SV_WAIT_AB
    sv_wait_bind(cpu);
#if R8_SV_THRESHOLD_AB
    sv_threshold_reset();sv_rom_crc=sv_threshold_crc32(game,(uint32_t)n);
#endif
#if R8_SV_POLL_COST_AB
    sv_poll_reset();
#if R8_SV_POLL_REJECT_DIAG
    sv_poll_reject_reset();
#endif
#endif
#endif
#if R8_SV_CPU_PROBE
    sv_cpu_probe_reset();
#endif
#if R8_SV_SPLIT_PROFILE
    diag_frame=0;r8_sv_reset();
#endif
    fraction=0;memset(pixels,0,160*160*2);return true;
}
void r8_controls(const r8_input *s){
    unsigned p=n3_pad(s),v=0;
    if(p&R8_RIGHT)v|=1;if(p&R8_LEFT)v|=2;if(p&R8_DOWN)v|=4;if(p&R8_UP)v|=8;
    if(p&R8_B)v|=16;if(p&R8_A)v|=32;if(p&R8_SELECT)v|=64;if(p&R8_START)v|=128;
    controls_state_write(v);
#if R8_SV_FIXED_BOOT
    sv_fixed_capture(s,(uint8_t)v);
#endif
}
void r8_frame(void){
#if R8_SV_DEX_BATCH
    sv_dex_batch_frame();
#if R8_SV_IO_BATCH
    sv_io_batch_frame();
#if R8_SV_RELEASE_BATCH
    sv_release_frame();
#if R8_SV_PCFILTER
    sv_pcfilter_frame();
#if R8_SV_OPFILTER
    sv_opfilter_frame();
#endif
#endif
#endif
#endif
#if R8_SV_POLL_DIAG
    sv_poll_diag_frame();
#endif
#endif
#if R8_SV_DEX_DIAG
    sv_dex_frame();
#endif
#if R8_SV_FIXED_BOOT
    sv_fixed_frame();
#endif
#if R8_SV_WAIT_AB
#if R8_SV_THRESHOLD_AB
    sv_wait_frame(1);
#if R8_SV_FIXED_BOOT
    sv_threshold_frame(sv_fixed_mode());
#else
    sv_threshold_frame(SV_THRESHOLD_FORCE_MODE);
#endif
#elif R8_SV_POLL_COST_AB
    sv_wait_frame(1);sv_poll_frame(SV_POLL_FORCE_MODE);
#else
    sv_wait_frame(SV_WAIT_FORCE_MODE);
#endif
    uint64_t wait_cpu_before=diag.cpu_us;
#endif
#if R8_SV_CPU_PROBE
    sv_cpu_probe_frame();
#endif
#if R8_SV_SPLIT_PROFILE

#if R8_SV_WAIT_AB
    bool timed=true;
#else
    bool timed=((diag_frame/120)&1)!=0;
#endif
    uint32_t whole=r8_sv_now(),mark=0;
#endif

    for(unsigned chunk=0;chunk<256;chunk++){
#if R8_SV_POLL_DIAG
        sv_poll_diag_before(cpu);
#endif
#if R8_SV_DEX_DIAG
        uint32_t dex_begin=r8_sv_now();sv_dex_before(cpu,256);
        sv_dex_add_outer((uint32_t)(r8_sv_now()-dex_begin));
#endif
#if R8_SV_POLL_REJECT_DIAG
        sv_poll_reject_before(cpu);
#endif
#if R8_SV_SPLIT_PROFILE
        if(timed)mark=r8_sv_now();
#endif
#if R8_SV_BATCH
        vrEmu6502RunCycles(cpu,256);
#else
        for(unsigned i=0;i<256;i++)vrEmu6502Tick(cpu);
#endif
#if R8_SV_SPLIT_PROFILE
        if(timed){
            uint32_t elapsed=(uint32_t)(r8_sv_now()-mark);diag.cpu_us+=elapsed;
#if R8_SV_POLL_COST_AB
            sv_poll_add_cpu(elapsed);
#endif
            diag.blocks++;mark=r8_sv_now();
        }
#endif
        timer_exec(256);fraction+=256u*44100u;
        unsigned n=fraction/4000000u;fraction%=4000000u;
        uint8_t samples[8];sound_stream_update(samples,n*2);
        for(unsigned i=0;i<n;i++)output(samples[i*2]*512,samples[i*2+1]*512);
#if R8_SV_SPLIT_PROFILE
        if(timed){diag.peripheral_us+=(uint32_t)(r8_sv_now()-mark);diag.pcm+=n;}
#endif
        if((chunk&3)==3&&service)service();
    }
#if R8_SV_SPLIT_PROFILE
    if(timed)mark=r8_sv_now();
#endif
    uint8_t *regs=memorymap_getRegisters();unsigned scan=regs[XPOS]/4+regs[YPOS]*48,inner=regs[XPOS]&3,size=regs[XSIZE];if(size>160)size=160;
    for(unsigned y=0;y<160;y++){if(scan>=0x1fe0)scan-=0x1fe0;gpu_render_scanline(scan,pixels+y*160,inner,size);scan+=48;}
    if(Rd6502(0x2026)&1)*vrEmu6502Nmi(cpu)=IntRequested;
    sound_decrement();
#if R8_SV_WAIT_AB
    sv_wait_add_cpu(diag.cpu_us-wait_cpu_before);
#if R8_SV_DEX_BATCH
    sv_dex_batch_add_cpu(diag.cpu_us-wait_cpu_before);
#if R8_SV_IO_BATCH
    sv_io_batch_add_cpu(diag.cpu_us-wait_cpu_before);
#if R8_SV_RELEASE_BATCH
    sv_release_add_cpu(diag.cpu_us-wait_cpu_before);
#endif
#endif
#endif
#if R8_SV_THRESHOLD_AB
    sv_threshold_add_cpu(diag.cpu_us-wait_cpu_before);
#endif
#endif
#if R8_SV_SPLIT_PROFILE
    if(timed){diag.gpu_us+=(uint32_t)(r8_sv_now()-mark);diag.timed_us+=(uint32_t)(r8_sv_now()-whole);diag.timed_frames++;}
    else{diag.plain_us+=(uint32_t)(r8_sv_now()-whole);diag.plain_frames++;}
    diag_frame++;
#endif
}
void r8_render(uint16_t *dst){
#if R8_SV_GROUP_RENDER

    for(unsigned sy=0;sy<160;sy+=2){
        uint16_t *row=dst+(sy/2*3)*320;
        const uint16_t *src=pixels+sy*160;
        memset(row,0,40*2);memset(row+280,0,40*2);
        for(unsigned x=0;x<160;x+=2){
            unsigned dx=x/2*3+40;
            uint16_t a=src[x]&0x7fff,b=src[x+1]&0x7fff;
            row[dx]=a;row[dx+1]=a;row[dx+2]=b;
        }
        memcpy(row+320,row,320*2);
        row+=640;src+=160;
        memset(row,0,40*2);memset(row+280,0,40*2);
        for(unsigned x=0;x<160;x+=2){
            unsigned dx=x/2*3+40;
            uint16_t a=src[x]&0x7fff,b=src[x+1]&0x7fff;
            row[dx]=a;row[dx+1]=a;row[dx+2]=b;
        }
    }
#else
    memset(dst,0,320*240*2);

    for(unsigned y=0;y<240;y++)for(unsigned x=0;x<240;x++)dst[y*320+x+40]=pixels[(y*2/3)*160+x*2/3]&0x7fff;
#endif
}
const char *r8_error(void){return error;}
uint32_t r8_clock(void){return 4000000;}
uint32_t r8_frame_cycles(void){return 65536;}
uint16_t r8_pc(void){return cpu?vrEmu6502GetPC(cpu):0;}
uint8_t r8_peek(uint16_t a){return a<0x2000?Rd6502(a):0;}
void r8_restore_save(const uint8_t *d,size_t n){(void)d;(void)n;}
size_t r8_export_save(uint8_t *d,size_t n){(void)d;(void)n;return 0;}
