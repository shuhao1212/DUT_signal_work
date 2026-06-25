// speed_change.c 开头部分
#include "speed_change.h"
#include "fft.h"
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

// 创建汉宁窗，用于平滑帧边界，减少拼接噪声
static void create_hanning_window(float* window, int len) {
    for (int i = 0; i < len; i++) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (len - 1)));
    }
}

// 重叠相加法变速不变调处理（p/q倍速）
static float wrap_phase(float phase) {
    while (phase > M_PI) phase -= 2.0f * M_PI;
    while (phase < -M_PI) phase += 2.0f * M_PI;
    return phase;
}

int change_speed(const float* input, int input_len, float* output, int* output_len,
                 int p, int q, int frame_len) {
    if (frame_len <= 0) return -1;
    int analysis_hop = frame_len / 4;
    if (analysis_hop < 1) analysis_hop = 1;
    double stretch_ratio = (double)q / p;
    int synthesis_hop = (int)(analysis_hop * stretch_ratio + 0.5);
    if (synthesis_hop < 1) synthesis_hop = 1;

    float* window = (float*)malloc(frame_len * sizeof(float));
    if (!window) return -1;
    create_hanning_window(window, frame_len);

    int max_output_len = (int)((double)input_len * stretch_ratio) + frame_len * 2;
    float* temp_out = (float*)calloc(max_output_len, sizeof(float));
    float* window_sum = (float*)calloc(max_output_len, sizeof(float));
    float* real = (float*)malloc(frame_len * sizeof(float));
    float* imag = (float*)malloc(frame_len * sizeof(float));
    float* prev_phase = (float*)calloc(frame_len / 2 + 1, sizeof(float));
    float* sum_phase = (float*)calloc(frame_len / 2 + 1, sizeof(float));
    if (!temp_out || !window_sum || !real || !imag || !prev_phase || !sum_phase) {
        free(window);
        free(temp_out); free(window_sum); free(real); free(imag); free(prev_phase); free(sum_phase);
        return -1;
    }

    int input_pos = 0;
    int output_pos = 0;
    int frame_count = 0;

    while (input_pos + frame_len <= input_len) {
        for (int i = 0; i < frame_len; i++) {
            real[i] = input[input_pos + i] * window[i];
            imag[i] = 0.0f;
        }

        fft(real, imag, frame_len, 0);

        for (int k = 0; k <= frame_len / 2; k++) {
            float mag = sqrtf(real[k] * real[k] + imag[k] * imag[k]);
            float phase = atan2f(imag[k], real[k]);
            if (frame_count == 0) {
                sum_phase[k] = phase;
            } else {
                float omega = 2.0f * M_PI * k / frame_len;
                float delta = wrap_phase(phase - prev_phase[k] - omega * analysis_hop);
                float true_freq = omega + delta / analysis_hop;
                sum_phase[k] += true_freq * synthesis_hop;
            }
            prev_phase[k] = phase;
            real[k] = mag * cosf(sum_phase[k]);
            imag[k] = mag * sinf(sum_phase[k]);
        }
        for (int k = frame_len / 2 + 1; k < frame_len; k++) {
            real[k] = real[frame_len - k];
            imag[k] = -imag[frame_len - k];
        }

        fft(real, imag, frame_len, 1);

        for (int i = 0; i < frame_len; i++) {
            int pos = output_pos + i;
            if (pos >= max_output_len) break;
            temp_out[pos] += real[i] * window[i];
            window_sum[pos] += window[i] * window[i];
        }

        input_pos += analysis_hop;
        output_pos += synthesis_hop;
        frame_count++;
    }

    *output_len = output_pos + frame_len;
    if (*output_len > max_output_len) *output_len = max_output_len;

    for (int i = 0; i < *output_len; i++) {
        if (window_sum[i] > 1e-6f)
            output[i] = temp_out[i] / window_sum[i];
        else
            output[i] = 0.0f;
    }

    free(window);
    free(temp_out);
    free(window_sum);
    free(real);
    free(imag);
    free(prev_phase);
    free(sum_phase);
    return 0;
}
