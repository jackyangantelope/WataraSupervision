/* SPDX-License-Identifier: MIT */
#include "sv_fixed_boot.h"
#include <stdio.h>
#include <string.h>
#ifndef SV_FIXED_SELECTOR
#define SV_FIXED_SELECTOR 1
#endif
volatile uint32_t sv_fixed_selector __attribute__((used,section(".data.sv_fixed_selector")))=SV_FIXED_SELECTOR;
static int fixed_mode=-1;
static sv_fixed_event ring[SV_FIXED_RING];
static uint32_t head,tail,total_frames,total_events,total_dropped,completed;
static r8_input pending,last;
static uint8_t pending_mapped,last_mapped;
static bool have_last,have_clock;
static uint64_t input_hash,start_us;
static sv_fixed_stats stats;
bool sv_fixed_reset(void){
    uint32_t selector=sv_fixed_selector;fixed_mode=-1;
    if(selector!=1&&selector!=2)return false;
    fixed_mode=(int)selector-1;
    head=tail=total_frames=total_events=total_dropped=completed=0;
    memset(&pending,0,sizeof pending);memset(&last,0,sizeof last);memset(&stats,0,sizeof stats);
    pending_mapped=last_mapped=0;have_last=have_clock=false;
    input_hash=0xcbf29ce484222325ull;start_us=0;return true;
}
int sv_fixed_mode(void){return fixed_mode;}
const char *sv_fixed_name(void){
    int mode=fixed_mode;
    if(mode<0){uint32_t s=sv_fixed_selector;mode=s==1?0:s==2?1:-1;}
    return mode==0?"OFF":mode==1?"ON":"INVALID";
}
void sv_fixed_begin(uint64_t now){start_us=now;have_clock=true;}
void sv_fixed_capture(const r8_input *input,uint8_t mapped){pending=*input;pending_mapped=mapped;}
static void enqueue(unsigned kind,uint64_t elapsed){
    uint32_t seq=++total_events;
    if(head-tail>=SV_FIXED_RING){stats.dropped++;total_dropped++;return;}
    sv_fixed_event *e=&ring[head&(SV_FIXED_RING-1)];memset(e,0,sizeof *e);
    e->seq=seq;e->frame=total_frames;e->hash=input_hash;e->elapsed_us=elapsed;
    e->input=last;e->mapped=last_mapped;e->kind=(uint8_t)kind;head++;stats.queued++;
}
void sv_fixed_frame(void){
    total_frames++;stats.frames++;
    uint8_t bytes[12]={(uint8_t)pending.pads[0],(uint8_t)(pending.pads[0]>>8),(uint8_t)pending.pads[1],(uint8_t)(pending.pads[1]>>8),0};
    memcpy(bytes+4,pending.keys,6);bytes[10]=pending.modifiers;bytes[11]=pending_mapped;
    for(unsigned i=0;i<sizeof bytes;i++)input_hash=(input_hash^bytes[i])*0x100000001b3ull;
    bool changed=!have_last||pending.pads[0]!=last.pads[0]||pending.pads[1]!=last.pads[1]||memcmp(pending.keys,last.keys,6)||pending.modifiers!=last.modifiers||pending_mapped!=last_mapped;
    if(changed){
        unsigned kind=have_last?SV_FIXED_CHANGE:SV_FIXED_INITIAL;
        if(have_last)stats.changes++;else stats.initial++;
        last=pending;last_mapped=pending_mapped;have_last=true;enqueue(kind,0);
    }
}
void sv_fixed_completed(uint64_t now){
    if(completed==total_frames)return;completed=total_frames;
    if(total_frames==1||total_frames==60||total_frames==120||total_frames==240||total_frames==300){
        stats.marks++;enqueue(SV_FIXED_MARK,have_clock?now-start_us:0);
    }
}
bool sv_fixed_pop(sv_fixed_event *out){if(head==tail)return false;*out=ring[tail&(SV_FIXED_RING-1)];tail++;return true;}
void sv_fixed_take(sv_fixed_stats *out){
    *out=stats;out->pending=head-tail;out->total_frames=total_frames;out->total_events=total_events;out->total_dropped=total_dropped;out->hash=input_hash;memset(&stats,0,sizeof stats);
}
int sv_fixed_format(char *out,size_t cap,const sv_fixed_event *e){
    if(e->kind==SV_FIXED_MARK)return snprintf(out,cap,"R8_SV_MARK seq=%lu frame=%lu elapsed_us=%llu hash=%016llx\n",(unsigned long)e->seq,(unsigned long)e->frame,(unsigned long long)e->elapsed_us,(unsigned long long)e->hash);
    return snprintf(out,cap,"R8_SV_INPUT seq=%lu frame=%lu kind=%s pad0=%04x pad1=%04x keys=%02x%02x%02x%02x%02x%02x mods=%02x sv=%02x read2020=%02x hash=%016llx\n",(unsigned long)e->seq,(unsigned long)e->frame,e->kind==SV_FIXED_INITIAL?"initial":"change",(unsigned)e->input.pads[0],(unsigned)e->input.pads[1],(unsigned)e->input.keys[0],(unsigned)e->input.keys[1],(unsigned)e->input.keys[2],(unsigned)e->input.keys[3],(unsigned)e->input.keys[4],(unsigned)e->input.keys[5],(unsigned)e->input.modifiers,(unsigned)e->mapped,(unsigned)(e->mapped^0xffu),(unsigned long long)e->hash);
}
