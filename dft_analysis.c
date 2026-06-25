#include <stdio.h>
#include <math.h>

// 参数定义（避免与参数重名，改用大写且不易冲突的名字）
#define FS       3000.0   // 抽样频率 3 kHz
#define DFT_SIZE 512      // 点数

// ---------- 函数声明 ----------
void dft(const double* x_real, int N, double* X_real, double* X_imag);
void magnitude(const double* X_real, const double* X_imag,
               int N, double* mag);
void generate_signal(double* x, int N, double fs);
void plot_spectrum_ascii(const double* mag, int N, double fs);

/*
 * 函数：dft
 * 计算离散傅里叶变换 X(k) = sum_{n=0}^{N-1} x(n) * exp(-j*2*pi*k*n/N)
 */
void dft(const double* x_real, int N,
         double* X_real, double* X_imag) {
    for (int k = 0; k < N; k++) {
        X_real[k] = 0.0;
        X_imag[k] = 0.0;
        for (int n = 0; n < N; n++) {
            double angle = -2.0 * M_PI * k * n / N;
            X_real[k] += x_real[n] * cos(angle);
            X_imag[k] += x_real[n] * sin(angle);
        }
    }
}

/*
 * 函数：magnitude
 * 计算复数数组的幅值 |X(k)|
 */
void magnitude(const double* X_real, const double* X_imag,
               int N, double* mag) {
    for (int k = 0; k < N; k++) {
        mag[k] = sqrt(X_real[k] * X_real[k] +
                      X_imag[k] * X_imag[k]);
    }
}

/*
 * 函数：generate_signal
 * 生成抽样序列 x(n) = [1+cos(2π·100·n/fs)]·cos(2π·600·n/fs)
 */
void generate_signal(double* x, int N, double fs) {
    for (int n = 0; n < N; n++) {
        double t = n / fs;
        double modulation = 1.0 + cos(2.0 * M_PI * 100.0 * t);
        double carrier    = cos(2.0 * M_PI * 600.0 * t);
        x[n] = modulation * carrier;
    }
}

/*
 * 函数：plot_spectrum_ascii (字符幅度谱)
 * 将前 N/2 点的幅度按比例缩放成字符柱状图，并标出主要峰值
 */
void plot_spectrum_ascii(const double* mag, int N, double fs) {
    int half = N / 2;
    double max_mag = 0.0;
    for (int k = 0; k <= half; k++)
        if (mag[k] > max_mag) max_mag = mag[k];

    printf("\n======= 幅度谱字符图 (0 ~ %.0f Hz) =======\n", fs/2.0);
    printf("    频率(Hz)    |      幅度 (归一化柱状图)        |X(k)|\n");
    printf("--------------------------------------------------------\n");

    for (int k = 0; k <= half; k++) {
        double freq = k * fs / N;
        int bar = (int)(mag[k] / max_mag * 40);  // 最大40个字符
        printf("%10.1f   |", freq);
        for (int i = 0; i < bar; i++) printf("*");
        for (int i = bar; i < 40; i++) printf(" ");
        printf("| %.2f\n", mag[k]);
    }

    // 自动找出并标注主要峰值（幅度超过 max_mag*0.1 的局部极大值）
    printf("\n--------- 主要谱峰坐标 ---------\n");
    double threshold = 0.1 * max_mag;
    for (int k = 1; k < half - 1; k++) {
        if (mag[k] > threshold &&
            mag[k] >= mag[k-1] && mag[k] >= mag[k+1]) {
            printf("峰值 -> 频率: %.1f Hz, 幅度 |X(k)| = %.2f\n",
                   k * fs / N, mag[k]);
        }
    }
}

// ==================== 主函数 ====================
int main() {
    double x[DFT_SIZE];
    double X_real[DFT_SIZE], X_imag[DFT_SIZE], mag[DFT_SIZE];

    // 1. 生成信号
    generate_signal(x, DFT_SIZE, FS);

    // 2. 计算 DFT
    dft(x, DFT_SIZE, X_real, X_imag);

    // 3. 计算幅度谱
    magnitude(X_real, X_imag, DFT_SIZE, mag);

    // 4. 输出数值表（前一半）
    printf("k\tFrequency (Hz)\t|X(k)|\n");
    printf("---------------------------------\n");
    for (int k = 0; k <= DFT_SIZE/2; k++) {
        double freq = k * FS / DFT_SIZE;
        printf("%d\t%.2f\t\t%.4f\n", k, freq, mag[k]);
    }

    // 5. 调用字符画函数
    plot_spectrum_ascii(mag, DFT_SIZE, FS);

    return 0;
}