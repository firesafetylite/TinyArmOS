/* TinyGPT navigation-key repeat. Printable/password keys are never synthesized. */
#ifndef TINYGPT_KEY_REPEAT_H
#define TINYGPT_KEY_REPEAT_H
#include <stdint.h>
typedef struct {
    uint8_t held[4];
    uint16_t active;
    uint64_t deadline;
} NativeKeyRepeat;
static uint64_t native_repeat_delay(uint64_t hz, int initial) {
    uint64_t delay=initial ? (hz/5U)*2U : hz/20U; /* 400 ms, then 20 Hz */
    return delay ? delay : 1;
}
static void native_repeat_event(NativeKeyRepeat *state, uint16_t scan, uint32_t value, uint64_t now, uint64_t hz) {
    if (scan<1 || scan>4 || value>2) return;
    if (!value) {
        state->held[scan-1]=0;
        if (state->active==scan) {
            state->active=0;
            for (uint16_t i=0;i<4;i++) if (state->held[i]) state->active=i+1;
            state->deadline=now+native_repeat_delay(hz,1);
        }
    } else if (value==1) {
        int initial=!state->held[scan-1];
        state->held[scan-1]=1;
        state->active=scan;
        state->deadline=now+native_repeat_delay(hz,initial);
    } else if (state->active==scan) {
        /* A host-generated repeat postpones ours, preventing double repeats. */
        state->deadline=now+native_repeat_delay(hz,0);
    }
}
static int native_repeat_poll(NativeKeyRepeat *state, uint64_t now, uint64_t hz, uint16_t *scan, uint16_t *character) {
    if (!state->active || (int64_t)(now-state->deadline)<0) return 0;
    *scan=state->active; *character=0;
    /* Never catch up with a burst after a slow redraw. */
    state->deadline=now+native_repeat_delay(hz,0);
    return 1;
}
#endif
