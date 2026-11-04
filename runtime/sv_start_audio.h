/* SPDX-License-Identifier: MIT */
#ifndef SV_START_AUDIO_H
#define SV_START_AUDIO_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
typedef struct {uint64_t us,accepted,taken,dropped,fill;uint32_t frame;unsigned q,kind;bool buffering;} sv_audio_edge;
typedef struct {
 bool active;uint32_t seen,used,head,frames;uint64_t start,last,gap;
 uint64_t offered,accepted,dropped,taken,fill,under,services;
 sv_audio_edge events[8];
} sv_start_audio;
static sv_start_audio sa;

static inline void sa_event(unsigned kind,uint64_t now,uint32_t frame,unsigned q,bool buffering){
 if(!sa.active||(sa.seen&(1u<<kind)))return;
 sa.seen|=1u<<kind;
 if(sa.used<8)sa.events[sa.used++]=(sv_audio_edge){now-sa.start,sa.accepted,sa.taken,sa.dropped,sa.fill,frame,q,kind,buffering};
}
static inline void sa_start(bool enabled,uint64_t now){memset(&sa,0,sizeof sa);sa.active=enabled;sa.start=now;sa.last=now;sa_event(0,now,0,0,true);}
static inline void sa_service(uint64_t now,uint32_t frame,unsigned q,bool buffering){
 if(!sa.active)return;
 uint64_t gap=now-sa.last;sa.last=now;sa.services++;
 if(gap>sa.gap)sa.gap=gap;
 sa_event(1,now,frame,q,buffering);if(gap>5000)sa_event(5,now,frame,q,buffering);
}
#endif
