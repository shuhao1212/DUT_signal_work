// noise_reduction.c
#include "noise_reduction.h"
#include "fft.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FRAME_SIZE 512
#define HOP_SIZE   256

int spectral_subtraction(const float* input, int input_len, 
                         int sample_rate, float** output, int* output_len) {
    int num_frames = (input_len - FRAME_SIZE) / HOP_SIZE + 1;
    if (num_frames < 1) return -1;

    int candidate_frames = (sample_rate == 16000) ? 32 : 16;
    if (candidate_frames > num_frames) candidate_frames = num_frames;

    float* frame_buf = (float*)malloc(FRAME_SIZE * sizeof(float));
    float* real = (float*)malloc(FRAME_SIZE * sizeof(float));
    float* imag = (float*)malloc(FRAME_SIZE * sizeof(float));
    if (!frame_buf || !real || !imag) {
        free(frame_buf); free(real); free(imag);
        return -1;
    }

    float* window = (float*)malloc(FRAME_SIZE * sizeof(float));
    if (!window) { free(frame_buf); free(real); free(imag); return -1; }
    for (int i = 0; i < FRAME_SIZE; i++)
        window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FRAME_SIZE - 1)));

    float* energies = (float*)malloc(candidate_frames * sizeof(float));
    int* indices = (int*)malloc(candidate_frames * sizeof(int));
    float* temp_mag = (float*)malloc(candidate_frames * (FRAME_SIZE/2 + 1) * sizeof(float));
    if (!energies || !indices || !temp_mag) {
        free(frame_buf); free(real); free(imag); free(window);
        free(energies); free(indices); free(temp_mag);
        return -1;
    }

    for (int f = 0; f < candidate_frames; f++) {
        int start = f * HOP_SIZE;
        memcpy(frame_buf, input + start, FRAME_SIZE * sizeof(float));
        for (int i = 0; i < FRAME_SIZE; i++) { real[i] = frame_buf[i] * window[i]; imag[i] = 0.0f; }
        fft(real, imag, FRAME_SIZE, 0);

        float energy = 0.0f;
        for (int k = 0; k <= FRAME_SIZE/2; k++) {
            float mag = sqrtf(real[k] * real[k] + imag[k] * imag[k]);
            temp_mag[f * (FRAME_SIZE/2 + 1) + k] = mag;
            energy += mag * mag;
        }
        energies[f] = energy;
        indices[f] = f;
    }

    for (int i = 0; i < candidate_frames - 1; i++) {
        int min_idx = i;
        for (int j = i + 1; j < candidate_frames; j++) {
            if (energies[j] < energies[min_idx])
                min_idx = j;
        }
        if (min_idx != i) {
            int tmp = indices[i];
            indices[i] = indices[min_idx];
            indices[min_idx] = tmp;
        }
    }
    int noise_frames = candidate_frames / 2;
    if (noise_frames < 2) noise_frames = 2;

    float* noise_mag = (float*)calloc(FRAME_SIZE/2 + 1, sizeof(float));
    if (!noise_mag) {
        free(frame_buf); free(real); free(imag); free(window);
        free(energies); free(indices); free(temp_mag);
        return -1;
    }

    for (int j = 0; j < noise_frames; j++) {
        int idx = indices[j];
        for (int k = 0; k <= FRAME_SIZE/2; k++)
            noise_mag[k] += temp_mag[idx * (FRAME_SIZE/2 + 1) + k];
    }
    for (int k = 0; k <= FRAME_SIZE/2; k++)
        noise_mag[k] /= noise_frames;

    for (int k = 1; k < FRAME_SIZE/2; k++) {
        noise_mag[k] = (noise_mag[k-1] + noise_mag[k] + noise_mag[k+1]) / 3.0f;
    }
    
    *output_len = num_frames * HOP_SIZE + FRAME_SIZE;
    float* out_buf = (float*)calloc(*output_len, sizeof(float));
    float* win_sum = (float*)calloc(*output_len, sizeof(float));
    if (!out_buf || !win_sum) {
        free(frame_buf); free(real); free(imag); free(window);
        free(energies); free(indices); free(temp_mag); free(noise_mag);
        free(out_buf); free(win_sum);
        return -1;
    }

    for (int f = 0; f < num_frames; f++) {
        int start = f * HOP_SIZE;
        memcpy(frame_buf, input + start, FRAME_SIZE * sizeof(float));
        for (int i = 0; i < FRAME_SIZE; i++) { real[i] = frame_buf[i] * window[i]; imag[i] = 0.0f; }
        fft(real, imag, FRAME_SIZE, 0);

        for (int k = 0; k <= FRAME_SIZE/2; k++) {
            float mag = sqrtf(real[k] * real[k] + imag[k] * imag[k]);
            float phase = atan2f(imag[k], real[k]);
            float oversub = 1.0f;
            float floor_gain = 0.10f;
            float target = mag - oversub * noise_mag[k];
            float min_mag = floor_gain * noise_mag[k];
            if (target < min_mag) target = min_mag;
            real[k] = target * cosf(phase);
            imag[k] = target * sinf(phase);
        }

        for (int k = FRAME_SIZE/2 + 1; k < FRAME_SIZE; k++) {
            real[k] = real[FRAME_SIZE - k];
            imag[k] = -imag[FRAME_SIZE - k];
        }
        fft(real, imag, FRAME_SIZE, 1);

        for (int i = 0; i < FRAME_SIZE; i++) {
            int pos = start + i;
            if (pos >= *output_len) break;
            out_buf[pos] += real[i] * window[i];
            win_sum[pos] += window[i] * window[i];
        }
    }

    for (int i = 0; i < *output_len; i++) {
        if (win_sum[i] > 1e-6f)
            out_buf[i] /= win_sum[i];
    }

    float max_val = 0.0f;
    for (int i = 0; i < *output_len; i++) {
        float abs_val = fabsf(out_buf[i]);
        if (abs_val > max_val) max_val = abs_val;
    }
    if (max_val > 0.0f) {
        for (int i = 0; i < *output_len; i++)
            out_buf[i] /= max_val;
    }

    free(frame_buf); free(real); free(imag); free(window);
    free(energies); free(indices); free(temp_mag); free(noise_mag); free(win_sum);
    *output = out_buf;
    return 0;
}