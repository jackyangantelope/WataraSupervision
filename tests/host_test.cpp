/* SPDX-License-Identifier: BSD-3-Clause */
#include "retro8.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
static uint64_t samples,nonzero,changes,service_calls,hash=1469598103934665603ull;
static int16_t lastl,lastr;static FILE *pcm_file;
static void audio(int16_t l,int16_t r){samples++;if(l||r)nonzero++;if(l!=lastl||r!=lastr)changes++;lastl=l;lastr=r;if(pcm_file){int16_t pair[]={l,r};fwrite(pair,2,2,pcm_file);}}
static void service(){service_calls++;}
static std::vector<uint8_t> read(const char *p){FILE*f=fopen(p,"rb");if(!f){perror(p);exit(2);}fseek(f,0,SEEK_END);size_t n=ftell(f);rewind(f);std::vector<uint8_t>d(n);if(fread(d.data(),1,n,f)!=n)exit(2);fclose(f);return d;}
static void snapshot(const std::string&name,const uint16_t *p){FILE*f=fopen(name.c_str(),"wb");fprintf(f,"P6\n320 240\n255\n");for(int i=0;i<320*240;i++){uint8_t rgb[]={(uint8_t)((p[i]&31)*255/31),(uint8_t)(((p[i]>>5)&31)*255/31),(uint8_t)(((p[i]>>10)&31)*255/31)};fwrite(rgb,1,3,f);}fclose(f);}
int main(int argc,char**argv){
    if(argc!=6){fprintf(stderr,"game bios-or-dash prefix mode frames\n");return 2;}
    auto game=read(argv[1]);std::vector<uint8_t>bios;if(strcmp(argv[2],"-"))bios=read(argv[2]);
    std::vector<uint8_t> bulk(2*1024*1024+128,0xa5);std::vector<uint16_t>pixels(320*240);
    if(!r8_init(bulk.data()+64,2*1024*1024,audio,service))return 3;

    if(r8_load(game.data(),1,bios.data(),bios.size(),argv[1]))return 4;
    if(!r8_load(game.data(),game.size(),bios.data(),bios.size(),argv[1])){fprintf(stderr,"LOAD: %s pc=%04x\n",r8_error(),r8_pc());return 5;}
    samples=nonzero=changes=service_calls=0;std::string prefix=argv[3],mode=argv[4];pcm_file=fopen((prefix+".pcm").c_str(),"wb");
    uint64_t cycles=0;int frames=atoi(argv[5]);
    for(int f=0;f<frames;f++){
        r8_input input={};
        if(mode!="idle"){

            unsigned p=0;if(f>=600&&f%180<30)p|=R8_A;
            if(strstr(argv[1],".sv")&&f>=300&&f<330)p|=R8_START;
            if(f>=700&&f%240<100)p|=(f/240%2?R8_LEFT:R8_RIGHT);
            if(f>=900&&f%240>=180)p|=R8_UP;
            if(strstr(argv[1],".lnx")){

                p=0;
                if(f>=600&&f<2400&&f%180<30)p=R8_A;
                if(f>=2400){if(f%240<30)p=R8_A;else if(f%240<150)p=R8_RIGHT;}
            }
            if(mode=="explore"){
                const unsigned trial[]={R8_A,R8_A,R8_B,R8_START,R8_SELECT,R8_Y,R8_RIGHT,R8_A};
                int index=(f-600)/300;p=0;
                if(f>=600&&index<8&&(f-600)%300<30)p=trial[index];
                if(f>=2400)p|=R8_RIGHT;
            }
            if(mode=="keyboard"){
                int n=0;if(p&R8_A)input.keys[n++]=0x1d;if(p&R8_B)input.keys[n++]=0x1b;if(p&R8_START)input.keys[n++]=0x28;if(p&R8_LEFT)input.keys[n++]=0x50;if(p&R8_RIGHT)input.keys[n++]=0x4f;if(p&R8_UP)input.keys[n++]=0x52;
            }else input.pads[0]=p;
        }
        r8_controls(&input);r8_frame();cycles+=r8_frame_cycles();
        if(r8_error()[0]){fprintf(stderr,"FRAME %d: %s\n",f,r8_error());return 6;}
        if(f%100==99||f==frames-1){r8_render(pixels.data());for(auto v:pixels){hash^=v;hash*=1099511628211ull;}}
        if(f==599||f==1199||f==frames-1){r8_render(pixels.data());snapshot(prefix+"-"+std::to_string(f+1)+".ppm",pixels.data());}
        if(mode=="explore"&&f%300==299){r8_render(pixels.data());snapshot(prefix+"-"+std::to_string(f+1)+".ppm",pixels.data());}
        if(strstr(argv[1],".lnx")&&f%600==599){r8_render(pixels.data());snapshot(prefix+"-"+std::to_string(f+1)+".ppm",pixels.data());}
    }
    fclose(pcm_file);pcm_file=nullptr;
    for(int i=0;i<64;i++)if(bulk[i]!=0xa5||bulk[bulk.size()-1-i]!=0xa5)return 7;
    uint64_t ramhash=1469598103934665603ull;for(unsigned i=0;i<8192;i++){ramhash^=r8_peek(i);ramhash*=1099511628211ull;}
    printf("RESULT mode=%s frames=%d cycles=%llu clock=%u samples=%llu nonzero=%llu changes=%llu video=%016llx ram=%016llx pc=%04x service=%llu\n",mode.c_str(),frames,(unsigned long long)cycles,r8_clock(),(unsigned long long)samples,(unsigned long long)nonzero,(unsigned long long)changes,(unsigned long long)hash,(unsigned long long)ramhash,r8_pc(),(unsigned long long)service_calls);
    if((mode!="idle"&&changes<20)||service_calls<100)return 8;
    uint8_t nv[2048],nv_after[2048];size_t nv_size=r8_export_save(nv,sizeof nv);
    r8_restore_save(nv,nv_size);
    if(!r8_load(game.data(),game.size(),bios.data(),bios.size(),argv[1]))return 9;
    if(nv_size){
        size_t after=r8_export_save(nv_after,sizeof nv_after);
        if(after!=nv_size||memcmp(nv,nv_after,after))return 10;
        printf("EEPROM_ROUNDTRIP_OK bytes=%zu\n",nv_size);
    }
    for(int f=0;f<10;f++)r8_frame();
    puts("RELOAD_OK");return 0;
}
