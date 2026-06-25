// student_dsp.c
// 进阶实验 IIR低通滤波器，330Hz方波 -> 正弦波
// 采样率48kHz，块长256，24位右对齐音频格式

#include "student_dsp.h"
#include <stdint.h>

#define IIR_SECTIONS 3          // 6阶Butterworth：3个二阶节

// 6阶Butterworth低通滤波器，截止频率约500Hz
// 使用直接II型二阶节级联结构
static const float iir_sos[IIR_SECTIONS][6] = {
    {1.084620721734e-09f, 2.175652826535e-09f, 1.091044770629e-09f,
     1.000000000000e+00f, -1.877130900731e+00f, 8.811586061385e-01f},
    {1.000000000000e+00f, 1.999969278310e+00f, 9.999808654617e-01f,
     1.000000000000e+00f, -1.907501631482e+00f, 9.115945018113e-01f},
    {1.000000000000e+00f, 1.994119546531e+00f, 9.941310430568e-01f,
     1.000000000000e+00f, -1.962497521032e+00f, 9.667083945929e-01f}
};

static float iir_state[IIR_SECTIONS][2] = {{0.0f}};

static float iir_filter(float input) {
    int section;
    float x = input;

    for (section = 0; section < IIR_SECTIONS; section++) {
        float b0 = iir_sos[section][0];
        float b1 = iir_sos[section][1];
        float b2 = iir_sos[section][2];
        float a1 = iir_sos[section][4];
        float a2 = iir_sos[section][5];
        float w0;
        float y;

        w0 = x - a1 * iir_state[section][0] - a2 * iir_state[section][1];
        y = b0 * w0 + b1 * iir_state[section][0] + b2 * iir_state[section][1];

        iir_state[section][1] = iir_state[section][0];
        iir_state[section][0] = w0;

        x = y;
    }

    return x;
}

void student_process_audio_block(volatile int32_t *inL,
                                 volatile int32_t *inR,
                                 volatile int32_t *outL,
                                 volatile int32_t *outR,
                                 uint16_t len) {
    uint16_t i;
    const float scale_in = 1.0f / 8388608.0f;
    const float scale_out = 8388607.0f;
    float x;
    float y;
    int32_t out_val;

    for (i = 0; i < len; i++) {
        x = (float)inL[i] * scale_in;
        y = iir_filter(x);
        out_val = (int32_t)(y * scale_out);

        if (out_val > 8388607) {
            out_val = 8388607;
        }
        if (out_val < -8388608) {
            out_val = -8388608;
        }

        outL[i] = out_val;
        outR[i] = out_val;
    }
}