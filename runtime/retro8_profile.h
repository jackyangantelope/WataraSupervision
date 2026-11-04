/* SPDX-License-Identifier: BSD-3-Clause */
// Drop whole messages when the UART queue fills.
#ifndef JACK_RETRO8_PROFILE_H
#define JACK_RETRO8_PROFILE_H
#if R8_PROFILE
#include <stdarg.h>
#include "hardware/uart.h"
#ifndef R8_PROFILE_UART_WRITE

#define R8_PROFILE_UART_WRITE(c) (uart_get_hw(uart_get_instance(PICO_DEFAULT_UART))->dr=(uint8_t)(c))
#endif
static struct {
    uint64_t start_us, service_us, usb_us, pad_us, audio_us;
    uint64_t emu_us, render_us, work_us;
    uint32_t completed, shown, service_calls;
} r8_profile;
#ifndef R8_PROFILE_TX_CAPACITY
#define R8_PROFILE_TX_CAPACITY 1536
#endif
static char r8_tx[R8_PROFILE_TX_CAPACITY],r8_format[768];
static size_t r8_tx_pos,r8_tx_size;
static uint32_t r8_tx_drops;
#if R8_BEEPER_STEADY
static r8_beeper_stats r8_beeper_start;
#endif
static void r8_profile_pump(void) {

    for(unsigned n=0;n<16&&r8_tx_pos<r8_tx_size;n++) {
        if(!uart_is_writable(uart_get_instance(PICO_DEFAULT_UART)))break;
        R8_PROFILE_UART_WRITE(r8_tx[r8_tx_pos++]);
    }
    if(r8_tx_pos==r8_tx_size)r8_tx_pos=r8_tx_size=0;
}
static void r8_profile_printf(const char *format,...) {
    va_list args;va_start(args,format);
    int n=vsnprintf(r8_format,sizeof r8_format,format,args);va_end(args);
    size_t pending=r8_tx_size-r8_tx_pos;
    if(n<0||(size_t)n>=sizeof r8_format||(size_t)n>sizeof r8_tx-pending){r8_tx_drops++;return;}
    memmove(r8_tx,r8_tx+r8_tx_pos,pending);
    memcpy(r8_tx+pending,r8_format,(size_t)n);
    r8_tx_pos=0;r8_tx_size=pending+(size_t)n;
    r8_profile_pump();
}
static void r8_profile_reset(uint64_t now) {
    memset(&r8_profile,0,sizeof r8_profile);r8_profile.start_us=now;
#if R8_BEEPER_STEADY
    r8_beeper_get(&r8_beeper_start);
#endif
#if R8_ZX_SPLIT_PROFILE
    r8_zx_split_reset();
#endif
}
static unsigned long r8_profile_average(uint64_t total,uint32_t n) {
    return n?(unsigned long)(total/n):0;
}
static void r8_profile_report(uint32_t frames,uint32_t max_us,uint32_t slow,
                              unsigned count,uint32_t under,uint32_t over,
                              uint32_t resync,unsigned pc) {
    uint64_t now=time_us_64(),elapsed=now-r8_profile.start_us;
    jack_nespad_input_stats pad={0};jack_nespad_input_get_stats(&pad);
    r8_profile_printf("R8_PERF frames=%lu max_us=%lu slow=%lu audio_q=%u under=%lu over=%lu resync=%lu pc=%04x\n"
        "R8_PROFILE variant=" R8_VARIANT " interval_us=%llu frames=%lu fps_milli=%lu shown=%lu "
        "emu_avg_us=%lu render_avg_us=%lu work_avg_us=%lu svc_us=%llu usb_us=%llu pad_us=%llu audio_us=%llu "
        "svc_calls=%lu tx_drop=%lu pad_start=%lu pad_done=%lu pad_max_us=%lu\n",
        (unsigned long)frames,(unsigned long)max_us,(unsigned long)slow,count,
        (unsigned long)under,(unsigned long)over,(unsigned long)resync,pc,
        (unsigned long long)elapsed,(unsigned long)r8_profile.completed,
        elapsed?(unsigned long)((uint64_t)r8_profile.completed*1000000000ull/elapsed):0,
        (unsigned long)r8_profile.shown,
        r8_profile_average(r8_profile.emu_us,r8_profile.completed),
        r8_profile_average(r8_profile.render_us,r8_profile.shown),
        r8_profile_average(r8_profile.work_us,r8_profile.completed),
        (unsigned long long)r8_profile.service_us,(unsigned long long)r8_profile.usb_us,
        (unsigned long long)r8_profile.pad_us,(unsigned long long)r8_profile.audio_us,
        (unsigned long)r8_profile.service_calls,(unsigned long)r8_tx_drops,
        (unsigned long)pad.starts,(unsigned long)pad.completed,(unsigned long)pad.max_latency_us);
#if R8_BEEPER_STEADY
    r8_beeper_stats beep;r8_beeper_get(&beep);
    uint32_t fast=beep.fast_ticks-r8_beeper_start.fast_ticks;
    uint32_t slow_ticks=beep.slow_ticks-r8_beeper_start.slow_ticks;
    uint64_t total=(uint64_t)fast+slow_ticks;
    r8_profile_printf("R8_BEEPER variant=" R8_VARIANT " frames=%lu fast_ticks=%lu slow_ticks=%lu "
        "entries=%lu fast_permille=%lu valid=%u\n",(unsigned long)r8_profile.completed,
        (unsigned long)fast,(unsigned long)slow_ticks,(unsigned long)(beep.entries-r8_beeper_start.entries),
        total?(unsigned long)((uint64_t)fast*1000/total):0,
        (unsigned)(total==(uint64_t)r8_profile.completed*r8_frame_cycles()));
#endif
#if R8_ZX_SPLIT_PROFILE
    r8_zx_split_stats zx;r8_zx_split_get(&zx);
    bool invalid=zx.ula_us>r8_profile.emu_us;
    r8_profile_printf("R8_ZX_SPLIT variant=" R8_VARIANT " frames=%lu ula_avg_us=%lu "
        "active_avg_us=%lu border_avg_us=%lu blank_avg_us=%lu other_avg_us=%lu "
        "ula_permille=%lu calls=%lu active_rows=%lu border_rows=%lu blank_rows=%lu "
        "max_line_us=%lu invalid=%u\n",
        (unsigned long)r8_profile.completed,
        r8_profile_average(zx.ula_us,r8_profile.completed),
        r8_profile_average(zx.active_us,r8_profile.completed),
        r8_profile_average(zx.border_us,r8_profile.completed),
        r8_profile_average(zx.blank_us,r8_profile.completed),
        r8_profile_average(invalid?0:r8_profile.emu_us-zx.ula_us,r8_profile.completed),
        r8_profile.emu_us?(unsigned long)(zx.ula_us*1000/r8_profile.emu_us):0,
        (unsigned long)zx.calls,(unsigned long)zx.active_rows,
        (unsigned long)zx.border_rows,(unsigned long)zx.blank_rows,
        (unsigned long)zx.max_line_us,(unsigned)invalid);
#endif
#if R8_SV_SPLIT_PROFILE
    r8_sv_stats sv;r8_sv_get(&sv);
    r8_profile_printf("R8_SV_SPLIT timed=%lu plain=%lu cpu_us=%llu peripheral_pcm_us=%llu gpu_tail_us=%llu timed_wall_us=%llu plain_wall_us=%llu blocks=%lu pcm=%lu\n",
        (unsigned long)sv.timed_frames,(unsigned long)sv.plain_frames,
        (unsigned long long)sv.cpu_us,(unsigned long long)sv.peripheral_us,
        (unsigned long long)sv.gpu_us,(unsigned long long)sv.timed_us,(unsigned long long)sv.plain_us,
        (unsigned long)sv.blocks,(unsigned long)sv.pcm);
    r8_sv_reset();
#endif
#if R8_SV_ROM_CACHE
    r8_profile_printf("R8_SV_ROM fills=%lu bytes_per_fill=16384 heap_tail=%lu\n",(unsigned long)r8_sv_cache_fills(),(unsigned long)r8_sv_heap_tail());
#endif
#if R8_SV_CPU_PROBE
    sv_cpu_probe_stats cp;sv_cpu_probe_take(&cp);
    r8_profile_printf("R8_SV_CPU samples=%lu eligible=%lu eligible_on=%lu on=%lu off=%lu total_cy=%llu read_cy=%llu write_cy=%llu rom_read_cy=%llu addr_ex_cy=%llu other_cy=%llu\n",
        (unsigned long)cp.samples,(unsigned long)cp.eligible,(unsigned long)cp.eligible_on,(unsigned long)cp.on_frames,(unsigned long)cp.off_frames,
        (unsigned long long)cp.total,(unsigned long long)cp.reads,(unsigned long long)cp.writes,
        (unsigned long long)cp.rom,(unsigned long long)cp.addr,(unsigned long long)cp.other);
    r8_profile_printf("R8_SV_CPU_CHECK reads=%lu writes=%lu modes=%lu bad=%lu max_cy=%lu empty_cy=%llu empty_calls=%lu\n",
        (unsigned long)cp.read_calls,(unsigned long)cp.write_calls,(unsigned long)cp.addr_calls,
        (unsigned long)cp.bad,(unsigned long)cp.max_cycles,(unsigned long long)cp.empty,(unsigned long)cp.empty_calls);
#endif
#if R8_SV_WAIT_AB
    sv_wait_stats wt;sv_wait_take(&wt);
#if R8_SV_POLL_COST_AB || R8_SV_THRESHOLD_AB
    r8_profile_printf("R8_SV_RAM on=%lu off=%lu cycles=%llu covered=%llu spans=%lu candidates=%lu irq_reject=%lu map_reject=%lu\n",
        (unsigned long)wt.on_frames,(unsigned long)wt.off_frames,(unsigned long long)wt.on_cycles,
        (unsigned long long)wt.skipped,(unsigned long)wt.spans,(unsigned long)wt.candidates,
        (unsigned long)wt.irq_reject,(unsigned long)wt.map_reject);
#if R8_SV_THRESHOLD_AB
    sv_threshold_stats th;sv_threshold_take(&th);
    r8_profile_printf("R8_SV_THRESH epoch=%lu on=%lu off=%lu on_cycles=%llu off_cycles=%llu covered=%llu spans=%lu candidates=%lu irq_reject=%lu map_reject=%lu on_cpu_us=%llu off_cpu_us=%llu\n",
        (unsigned long)th.epoch,(unsigned long)th.on_frames,(unsigned long)th.off_frames,
        (unsigned long long)th.on_cycles,(unsigned long long)th.off_cycles,(unsigned long long)th.covered,
        (unsigned long)th.spans,(unsigned long)th.candidates,(unsigned long)th.irq_reject,(unsigned long)th.map_reject,
        (unsigned long long)th.on_cpu_us,(unsigned long long)th.off_cpu_us);
#else
    sv_poll_stats pl;sv_poll_take(&pl);
    r8_profile_printf("R8_SV_POLL epoch=%lu on=%lu off=%lu io_on=%lu io_off=%lu other=%lu io_on_us=%llu io_off_us=%llu other_us=%llu\n",
        (unsigned long)pl.epoch,(unsigned long)pl.on_frames,(unsigned long)pl.off_frames,
        (unsigned long)pl.io_on_blocks,(unsigned long)pl.io_off_blocks,(unsigned long)pl.other_blocks,
        (unsigned long long)pl.io_on_us,(unsigned long long)pl.io_off_us,(unsigned long long)pl.other_us);
#endif
#else
    r8_profile_printf("R8_SV_WAIT epoch=%lu on=%lu off=%lu on_cycles=%llu off_cycles=%llu skipped=%llu spans=%lu candidates=%lu irq_reject=%lu map_reject=%lu on_cpu_us=%llu off_cpu_us=%llu\n",
        (unsigned long)wt.epoch,(unsigned long)wt.on_frames,(unsigned long)wt.off_frames,
        (unsigned long long)wt.on_cycles,(unsigned long long)wt.off_cycles,(unsigned long long)wt.skipped,
        (unsigned long)wt.spans,(unsigned long)wt.candidates,(unsigned long)wt.irq_reject,(unsigned long)wt.map_reject,
        (unsigned long long)wt.on_cpu_us,(unsigned long long)wt.off_cpu_us);
#endif
#endif
#if R8_SV_POLL_REJECT_DIAG
    sv_reject_stats rj;sv_poll_reject_take(&rj);
    r8_profile_printf("R8_SV_REJ calls=%lu samples=%lu on=%lu off=%lu other=%lu state=%lu irq=%lu acc=%lu phase=%lu bounds=%lu sig=%lu key=%lu pass=%lu us=%llu max_us=%lu pc=%04x wpc=%04x op=%02x a=%02x p=%02x k=%02x\n",
        (unsigned long)rj.calls,(unsigned long)rj.samples,(unsigned long)rj.on,(unsigned long)rj.off,
        (unsigned long)rj.other,(unsigned long)rj.state,(unsigned long)rj.irq,(unsigned long)rj.accum,
        (unsigned long)rj.phase,(unsigned long)rj.bounds,(unsigned long)rj.sig,(unsigned long)rj.key,(unsigned long)rj.pass,
        (unsigned long long)rj.us,(unsigned long)rj.max_us,(unsigned)rj.pc,(unsigned)rj.wpc,(unsigned)rj.op,(unsigned)rj.a,(unsigned)rj.p,(unsigned)rj.k);
#endif
#if R8_SV_FIXED_BOOT
    sv_fixed_stats fi;sv_fixed_take(&fi);
    r8_profile_printf("R8_SV_INPUT_SUM frames=%lu initial=%lu changes=%lu marks=%lu queued=%lu dropped=%lu pending=%lu total_frames=%lu total_events=%lu total_dropped=%lu hash=%016llx\n",
        (unsigned long)fi.frames,(unsigned long)fi.initial,(unsigned long)fi.changes,(unsigned long)fi.marks,
        (unsigned long)fi.queued,(unsigned long)fi.dropped,(unsigned long)fi.pending,(unsigned long)fi.total_frames,
        (unsigned long)fi.total_events,(unsigned long)fi.total_dropped,(unsigned long long)fi.hash);
#endif
#if R8_SV_DEX_DIAG
    sv_dex_stats dx;sv_dex_take(&dx);
    r8_profile_printf("R8_SV_DEX on=%lu off=%lu on_blocks=%lu off_blocks=%lu on_cycles=%llu off_cycles=%llu eligible_cycles=%llu eligible=%lu bne=%lu dex=%lu other=%lu phase=%lu state=%lu irq=%lu signature=%lu exit=%lu budget=%lu x_limited=%lu budget_limited=%lu align_cycles=%lu\n",
        (unsigned long)dx.on_frames,(unsigned long)dx.off_frames,(unsigned long)dx.on_blocks,(unsigned long)dx.off_blocks,
        (unsigned long long)dx.on_cycles,(unsigned long long)dx.off_cycles,(unsigned long long)dx.eligible_cycles,
        (unsigned long)dx.eligible,(unsigned long)dx.bne,(unsigned long)dx.dex,(unsigned long)dx.other,(unsigned long)dx.phase,
        (unsigned long)dx.state,(unsigned long)dx.irq,(unsigned long)dx.signature,(unsigned long)dx.exit,(unsigned long)dx.budget,
        (unsigned long)dx.x_limited,(unsigned long)dx.budget_limited,(unsigned long)dx.align_cycles);
    r8_profile_printf("R8_SV_DEX_TIME on_observer_us=%llu off_observer_us=%llu timed_checks=%lu check_us=%llu hit_us=%llu miss_us=%llu max_check_us=%lu empty_samples=%lu empty_us=%llu max_empty_us=%lu\n",
        (unsigned long long)dx.on_observer_us,(unsigned long long)dx.off_observer_us,(unsigned long)dx.timed_checks,
        (unsigned long long)dx.check_us,(unsigned long long)dx.hit_us,(unsigned long long)dx.miss_us,(unsigned long)dx.max_check_us,(unsigned long)dx.empty_samples,(unsigned long long)dx.empty_us,(unsigned long)dx.max_empty_us);
#endif
#if R8_SV_DEX_BATCH
    sv_dex_batch_stats db;sv_dex_batch_take(&db);
    r8_profile_printf("R8_SV_DBATCH on=%lu off=%lu on_blocks=%lu off_blocks=%lu on_cycles=%llu off_cycles=%llu batched_cycles=%llu blocks_with_batch=%lu spans=%lu candidates=%lu irq_reject=%lu map_reject=%lu guard_bypass=%lu on_cpu_us=%llu off_cpu_us=%llu\n",
        (unsigned long)db.on_frames,(unsigned long)db.off_frames,(unsigned long)db.on_blocks,(unsigned long)db.off_blocks,
        (unsigned long long)db.on_cycles,(unsigned long long)db.off_cycles,(unsigned long long)db.batched_cycles,
        (unsigned long)db.blocks_with_batch,(unsigned long)db.spans,(unsigned long)db.candidates,(unsigned long)db.irq_reject,
        (unsigned long)db.map_reject,(unsigned long)db.guard_bypass,(unsigned long long)db.on_cpu_us,(unsigned long long)db.off_cpu_us);
#endif
#if R8_SV_POLL_DIAG
    sv_poll_diag_stats pd;sv_poll_diag_take(&pd);
    r8_profile_printf("R8_SV_POLL blocks=%lu frames=%lu watched=%lu eligible=%lu other=%lu state=%lu phase=%lu irq=%lu signature=%lu key=%lu lda=%lu cmp=%lu beq=%lu reads=%lu ff=%lu nonff=%lu mismatch=%lu\n",(unsigned long)pd.blocks,(unsigned long)pd.frames,(unsigned long)pd.watched,(unsigned long)pd.eligible,(unsigned long)pd.other,(unsigned long)pd.state,(unsigned long)pd.phase,(unsigned long)pd.irq,(unsigned long)pd.signature,(unsigned long)pd.key,(unsigned long)pd.lda,(unsigned long)pd.cmp,(unsigned long)pd.beq,(unsigned long)pd.reads,(unsigned long)pd.ff,(unsigned long)pd.nonff,(unsigned long)pd.mismatch);
    r8_profile_printf("R8_SV_POLL_TIME check_us=%llu hit_us=%llu miss_us=%llu outer_us=%llu max_us=%lu empty_us=%llu empty_samples=%lu\n",(unsigned long long)pd.check_us,(unsigned long long)pd.hit_us,(unsigned long long)pd.miss_us,(unsigned long long)pd.outer_us,(unsigned long)pd.max_us,(unsigned long long)pd.empty_us,(unsigned long)pd.empty_samples);
    if(pd.watched){for(unsigned i=0;i<2;i++){const sv_poll_state *s=i?&pd.last:&pd.first;
      r8_profile_printf("R8_SV_POLL_STATE which=%u pc=%04x addr=%04x op=%02x step=%u a=%02x flags=%02x input=%02x irq=%u nmi=%u reason=%u\n",i,s->pc,s->addr,s->op,s->step,s->a,s->flags,s->input,s->irq,s->nmi,s->reason);
    }}
    r8_profile_printf("R8_SV_AUDIO active=%u offered=%llu accepted=%llu dropped=%llu taken=%llu fill=%llu under=%llu services=%llu max_gap_us=%llu edges=%lu pending=%lu q=%u\n",sa.active,(unsigned long long)sa.offered,(unsigned long long)sa.accepted,(unsigned long long)sa.dropped,(unsigned long long)sa.taken,(unsigned long long)sa.fill,(unsigned long long)sa.under,(unsigned long long)sa.services,(unsigned long long)sa.gap,(unsigned long)sa.used,(unsigned long)(sa.used-sa.head),count);
#endif
#if R8_SV_IO_BATCH
    sv_io_batch_stats ib;sv_io_batch_take(&ib);
    r8_profile_printf("R8_SV_IOBATCH on=%lu off=%lu on_blocks=%lu off_blocks=%lu on_cycles=%llu off_cycles=%llu batched_cycles=%llu blocks_with_batch=%lu spans=%lu candidates=%lu irq_reject=%lu map_reject=%lu guard_bypass=%lu on_cpu_us=%llu off_cpu_us=%llu\n",(unsigned long)ib.on_frames,(unsigned long)ib.off_frames,(unsigned long)ib.on_blocks,(unsigned long)ib.off_blocks,(unsigned long long)ib.on_cycles,(unsigned long long)ib.off_cycles,(unsigned long long)ib.batched_cycles,(unsigned long)ib.blocks_with_batch,(unsigned long)ib.spans,(unsigned long)ib.candidates,(unsigned long)ib.irq_reject,(unsigned long)ib.map_reject,(unsigned long)ib.guard_bypass,(unsigned long long)ib.on_cpu_us,(unsigned long long)ib.off_cpu_us);
#endif
#if R8_SV_RELEASE_BATCH
    sv_io_batch_stats rel;sv_release_take(&rel);
    r8_profile_printf("R8_SV_RELEASE on=%lu off=%lu on_blocks=%lu off_blocks=%lu on_cycles=%llu off_cycles=%llu batched_cycles=%llu blocks_with_batch=%lu spans=%lu candidates=%lu irq_reject=%lu map_reject=%lu guard_bypass=%lu on_cpu_us=%llu off_cpu_us=%llu\n",(unsigned long)rel.on_frames,(unsigned long)rel.off_frames,(unsigned long)rel.on_blocks,(unsigned long)rel.off_blocks,(unsigned long long)rel.on_cycles,(unsigned long long)rel.off_cycles,(unsigned long long)rel.batched_cycles,(unsigned long)rel.blocks_with_batch,(unsigned long)rel.spans,(unsigned long)rel.candidates,(unsigned long)rel.irq_reject,(unsigned long)rel.map_reject,(unsigned long)rel.guard_bypass,(unsigned long long)rel.on_cpu_us,(unsigned long long)rel.off_cpu_us);
#endif
#if R8_SV_PCFILTER
    sv_pcfilter_stats pf;sv_pcfilter_take(&pf);
    r8_profile_printf("R8_SV_PCFILTER on=%lu off=%lu ram_bypass=%lu rom_checks=%lu\n",(unsigned long)pf.on_frames,(unsigned long)pf.off_frames,(unsigned long)pf.ram_bypass,(unsigned long)pf.rom_checks);
#if R8_SV_OPFILTER
    sv_opfilter_stats of;sv_opfilter_take(&of);
    r8_profile_printf("R8_SV_OPFILTER on=%lu off=%lu bypassed=%lu eligible=%lu\n",(unsigned long)of.on_frames,(unsigned long)of.off_frames,(unsigned long)of.bypassed,(unsigned long)of.eligible);
#endif
#endif

    r8_profile_reset(now);
}
#else
#define r8_profile_printf printf
#endif
#endif
