// fft.c
#include "fft.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void bit_reverse(float* real, float* imag, int n) {
    for (int i = 0, j = 0; i < n; i++) {
        if (i > j) {
            float tr = real[i]; real[i] = real[j]; real[j] = tr;
            float ti = imag[i]; imag[i] = imag[j]; imag[j] = ti;
        }
        for (int m = n >> 1; (j ^= m) < m; m >>= 1);
    }
}

void fft(float* real, float* imag, int n, int inverse) {
    bit_reverse(real, imag, n);
    for (int len = 2; len <= n; len <<= 1) {
        float ang = 2 * M_PI / len * (inverse ? -1 : 1);
        float wlen_r = cosf(ang), wlen_i = sinf(ang);
        for (int i = 0; i < n; i += len) {
            float w_r = 1.0f, w_i = 0.0f;
            for (int j = 0; j < len/2; j++) {
                int a = i + j, b = i + j + len/2;
                float u_r = real[a], u_i = imag[a];
                float v_r = real[b]*w_r - imag[b]*w_i;
                float v_i = real[b]*w_i + imag[b]*w_r;
                real[a] = u_r + v_r; imag[a] = u_i + v_i;
                real[b] = u_r - v_r; imag[b] = u_i - v_i;
                float nw_r = w_r*wlen_r - w_i*wlen_i;
                float nw_i = w_r*wlen_i + w_i*wlen_r;
                w_r = nw_r; w_i = nw_i;
            }
        }
    }
    if (inverse) {
        for (int i = 0; i < n; i++) {
            real[i] /= n;
            imag[i] /= n;
        }
    }
}