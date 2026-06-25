/******************************************************************************
 * 双音多频(DTMF)信号分析 —— 基2 DIT/DIF FFT 实现 + BMP 图像输出
 * 
 * 编译: gcc -o dtmf t3.c -lm
 * 运行: ./dtmf
 * 输出文件:
 *   dit_output.txt       - FFT(DIT)原始复数结果
 *   dif_output.txt       - FFT(DIF)原始复数结果
 *   spectrum.txt         - 真实频率(Hz)与幅度(V)
 *   fft_raw.bmp          - FFT原始输出谱线图 (k vs 模值)
 *   spectrum.bmp         - 真实频谱图 (Hz vs V)
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

/* ---------- 复数结构 ---------- */
typedef struct {
    double re;
    double im;
} complex;

/* 复数运算 */
complex c_add(complex a, complex b) {
    complex res = {a.re + b.re, a.im + b.im};
    return res;
}
complex c_sub(complex a, complex b) {
    complex res = {a.re - b.re, a.im - b.im};
    return res;
}
complex c_mul(complex a, complex b) {
    complex res = {a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re};
    return res;
}
complex c_exp(double theta) {
    complex res = {cos(theta), sin(theta)};
    return res;
}

/* ---------- 位逆序排列 (基2 DIT需要) ---------- */
void bit_reverse(complex *x, int N) {
    int i, j, k;
    complex temp;
    j = 0;
    for (i = 0; i < N; i++) {
        if (i < j) {
            temp = x[i];
            x[i] = x[j];
            x[j] = temp;
        }
        k = N >> 1;
        while (j & k) {
            j ^= k;
            k >>= 1;
        }
        j ^= k;
    }
}

/* ---------- 基2 时间抽取(DIT) FFT ---------- */
void fft_dit(complex *x, int N) {
    int len, m, i, j;
    complex W, Wm, u, t;

    bit_reverse(x, N);

    for (len = 2; len <= N; len <<= 1) {
        m = len >> 1;
        Wm = c_exp(-2.0 * PI / len);
        for (i = 0; i < N; i += len) {
            W.re = 1.0; W.im = 0.0;
            for (j = 0; j < m; j++) {
                u = x[i + j];
                t = c_mul(W, x[i + j + m]);
                x[i + j] = c_add(u, t);
                x[i + j + m] = c_sub(u, t);
                W = c_mul(W, Wm);
            }
        }
    }
}

/* ---------- 基2 频率抽取(DIF) FFT ---------- */
void fft_dif(complex *x, int N) {
    int len, m, i, j;
    complex W, Wm, u, t;

    for (len = N; len >= 2; len >>= 1) {
        m = len >> 1;
        Wm = c_exp(-2.0 * PI / len);
        for (i = 0; i < N; i += len) {
            W.re = 1.0; W.im = 0.0;
            for (j = 0; j < m; j++) {
                u = x[i + j];
                t = x[i + j + m];
                x[i + j] = c_add(u, t);
                x[i + j + m] = c_mul(c_sub(u, t), W);
                W = c_mul(W, Wm);
            }
        }
    }
    bit_reverse(x, N);   // DIF输出为倒位序，重排为自然序
}

/* ---------- 生成DTMF数字“8”的采样信号 ---------- */
void generate_dtmf(complex *x, int M, double fs) {
    double f1 = 1336.0;
    double f2 = 852.0;
    double A = 0.25;
    double dt = 1.0 / fs;
    for (int n = 0; n < M; n++) {
        double t = n * dt;
        x[n].re = A * sin(2.0 * PI * f1 * t) + A * sin(2.0 * PI * f2 * t);
        x[n].im = 0.0;
    }
}

/* ---------- 计算FFT模值 ---------- */
void calc_amplitude(complex *fft_out, double *amp, int N) {
    for (int k = 0; k < N; k++) {
        amp[k] = sqrt(fft_out[k].re * fft_out[k].re + fft_out[k].im * fft_out[k].im);
    }
}

/* ---------- 保存复数结果（用于画原始输出谱线图） ---------- */
void save_complex(const char *filename, complex *data, int N) {
    FILE *fp = fopen(filename, "w");
    if (!fp) { perror("file open"); return; }
    for (int k = 0; k < N; k++)
        fprintf(fp, "%d %f %f\n", k, data[k].re, data[k].im);
    fclose(fp);
}

/* ---------- 保存真实频谱数据 ---------- */
void save_spectrum(const char *filename, double *amp, int N, double fs) {
    FILE *fp = fopen(filename, "w");
    if (!fp) { perror("file open"); return; }
    double df = fs / N;
    for (int k = 0; k <= N/2; k++) {
        double freq = k * df;
        double mag;
        if (k == 0 || k == N/2)
            mag = amp[k] / N;
        else
            mag = 2.0 * amp[k] / N;
        fprintf(fp, "%f %f\n", freq, mag);
    }
    fclose(fp);
}

/* ---------- 写入BMP文件头 ---------- */
void write_bmp_header(FILE *fp, int width, int height) {
    int filesize = 54 + 3 * width * height;
    unsigned char bmpfileheader[14] = {
        'B','M',
        filesize & 0xFF, (filesize>>8) & 0xFF,
        (filesize>>16) & 0xFF, (filesize>>24) & 0xFF,
        0,0, 0,0,
        54,0,0,0
    };
    unsigned char bmpinfoheader[40] = {
        40,0,0,0,
        width & 0xFF, (width>>8) & 0xFF,
        (width>>16) & 0xFF, (width>>24) & 0xFF,
        height & 0xFF, (height>>8) & 0xFF,
        (height>>16) & 0xFF, (height>>24) & 0xFF,
        1,0,
        24,0,
        0,0,0,0, 0,0,0,0,
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
    };
    fwrite(bmpfileheader, 1, 14, fp);
    fwrite(bmpinfoheader, 1, 40, fp);
}

/* ---------- 绘制FFT原始输出谱线图（k vs 模值，不归一化） ---------- */
void save_bmp_raw(const char *filename, double *amp, int N, int width, int height) {
    unsigned char *img = (unsigned char*)malloc(width * height * 3);
    memset(img, 255, width * height * 3);  // 白色背景

    double max_val = 0.0;
    for (int i = 0; i < N; i++)
        if (amp[i] > max_val) max_val = amp[i];
    if (max_val < 1e-6) max_val = 1.0;

    // 横轴为k (0 ~ N-1)
    for (int x = 0; x < width; x++) {
        int k = x * (N - 1) / (width - 1);
        double mag = amp[k];
        int bar_h = (int)(mag / max_val * height);
        if (bar_h > height) bar_h = height;

        // 红色竖线，宽2像素
        for (int w = 0; w < 2 && x+w < width; w++) {
            for (int y = 0; y < bar_h; y++) {
                int row = height - 1 - y;
                int idx = (row * width + x + w) * 3;
                img[idx + 0] = 0;   // B
                img[idx + 1] = 0;   // G
                img[idx + 2] = 255; // R
            }
        }
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) { free(img); return; }
    write_bmp_header(fp, width, height);
    fwrite(img, 3, width * height, fp);
    fclose(fp);
    free(img);
}

/* ---------- 绘制真实频谱图（Hz vs V） ---------- */
void save_bmp_spectrum(const char *filename, double *amp, int N, double fs,
                       int width, int height) {
    // 先归一化得到真实幅度
    double *real_amp = (double*)malloc((N/2+1) * sizeof(double));
    for (int k = 0; k <= N/2; k++) {
        if (k == 0 || k == N/2)
            real_amp[k] = amp[k] / N;
        else
            real_amp[k] = 2.0 * amp[k] / N;
    }

    unsigned char *img = (unsigned char*)malloc(width * height * 3);
    memset(img, 255, width * height * 3);  // 白色背景

    double max_amp = 0.0;
    for (int i = 0; i <= N/2; i++)
        if (real_amp[i] > max_amp) max_amp = real_amp[i];
    if (max_amp < 1e-6) max_amp = 0.25;

    double df = fs / N;
    // 横轴频率 0 ~ fs/2
    for (int x = 0; x < width; x++) {
        double freq = x * (fs / 2.0) / (width - 1);
        int k = (int)(freq / df + 0.5);
        if (k > N/2) k = N/2;
        double mag = real_amp[k];
        int bar_h = (int)(mag / max_amp * height);
        if (bar_h > height) bar_h = height;

        // 红色竖线，宽2像素
        for (int w = 0; w < 2 && x+w < width; w++) {
            for (int y = 0; y < bar_h; y++) {
                int row = height - 1 - y;
                int idx = (row * width + x + w) * 3;
                img[idx + 0] = 0;   // B
                img[idx + 1] = 0;   // G
                img[idx + 2] = 255; // R
            }
        }
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) { free(real_amp); free(img); return; }
    write_bmp_header(fp, width, height);
    fwrite(img, 3, width * height, fp);
    fclose(fp);
    free(real_amp);
    free(img);
}

/* ---------- 主程序 ---------- */
int main() {
    const double fs = 8000.0;            // 采样频率
    const int M = 512;                   // 采样点数
    const int N = 512;                   // FFT点数

    complex *x_dit = (complex*) malloc(N * sizeof(complex));
    complex *x_dif = (complex*) malloc(N * sizeof(complex));
    double *amp_dit = (double*) malloc(N * sizeof(double));
    double *amp_dif = (double*) malloc(N * sizeof(double));

    // 生成信号，剩余位置补零
    generate_dtmf(x_dit, M, fs);
    for (int i = M; i < N; i++) {
        x_dit[i].re = 0.0;
        x_dit[i].im = 0.0;
    }
    for (int i = 0; i < N; i++)
        x_dif[i] = x_dit[i];

    // FFT
    fft_dit(x_dit, N);
    fft_dif(x_dif, N);

    // 计算模值
    calc_amplitude(x_dit, amp_dit, N);
    calc_amplitude(x_dif, amp_dif, N);

    // 保存文本结果
    save_complex("dit_output.txt", x_dit, N);
    save_complex("dif_output.txt", x_dif, N);
    save_spectrum("spectrum.txt", amp_dit, N, fs);

    // 绘制BMP图像（用DIT的结果，DIF结果理论上一致）
    save_bmp_raw("fft_raw.bmp", amp_dit, N, 800, 400);
    save_bmp_spectrum("spectrum.bmp", amp_dit, N, fs, 800, 400);

    printf("Done. Output files:\n");
    printf("  dit_output.txt, dif_output.txt\n");
    printf("  spectrum.txt\n");
    printf("  fft_raw.bmp   (FFT raw spectrum, k vs |X|)\n");
    printf("  spectrum.bmp  (True spectrum, Hz vs V)\n");

    free(x_dit); free(x_dif); free(amp_dit); free(amp_dif);
    return 0;
}