#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// DTMF 频率表（行频+列频）
static const int dtmf_freq[8] = {
    697, 770, 852, 941,  // 行频
    1209, 1336, 1477, 1633 // 列频
};

// DTMF 字符映射表 4x4
static const char dtmf_map[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// Goertzel 算法：返回指定频率的幅度平方（避免开方，提高效率）
static double goertzel(const int16_t *samples, int n, int target_freq, int sample_rate) {
    double omega = 2.0 * M_PI * target_freq / sample_rate;
    double coeff = 2.0 * cos(omega);
    double q0 = 0.0, q1 = 0.0, q2 = 0.0;
    for (int i = 0; i < n; i++) {
        q0 = coeff * q1 - q2 + samples[i];
        q2 = q1;
        q1 = q0;
    }
    double real = q1 - q2 * cos(omega);
    double imag = q2 * sin(omega);
    return real * real + imag * imag;
}

// 检测一帧数据中的 DTMF 字符，若无效返回 0
static char detect_frame(const int16_t *frame, int frame_len, int sample_rate) {
    double mag[8];
    double max_row = 0.0, max_col = 0.0;
    int row_idx = -1, col_idx = -1;
    
    // 计算所有频率的幅度平方
    for (int i = 0; i < 8; i++) {
        mag[i] = goertzel(frame, frame_len, dtmf_freq[i], sample_rate);
        if (i < 4) {
            if (mag[i] > max_row) { max_row = mag[i]; row_idx = i; }
        } else {
            if (mag[i] > max_col) { max_col = mag[i]; col_idx = i - 4; }
        }
    }
    if (row_idx == -1 || col_idx == -1) return 0;
    
    // 找行频组第二高峰值
    double second_row = 0.0;
    for (int i = 0; i < 4; i++) {
        if (i != row_idx && mag[i] > second_row) second_row = mag[i];
    }
    // 找列频组第二高峰值
    double second_col = 0.0;
    for (int i = 4; i < 8; i++) {
        if ((i - 4) != col_idx && mag[i] > second_col) second_col = mag[i];
    }
    
    // 峰值必须显著高于组内第二峰值（至少高 2 倍）
    if (max_row < second_row * 2.0 || max_col < second_col * 2.0) return 0;
    
    // 绝对能量门限：防止静音时误判
    double total_energy = 0.0;
    for (int i = 0; i < frame_len; i++) {
        total_energy += (double)frame[i] * frame[i];
    }
    double peak_energy = max_row + max_col;
    if (peak_energy < total_energy * 0.001) return 0;
    
    // 可选：检查二次谐波（简单二次谐波功率应远小于基波）
    double second_harm_row = goertzel(frame, frame_len, 2 * dtmf_freq[row_idx], sample_rate);
    double second_harm_col = goertzel(frame, frame_len, 2 * dtmf_freq[4 + col_idx], sample_rate);
    if (second_harm_row > max_row * 0.5 || second_harm_col > max_col * 0.5) return 0;
    
    return dtmf_map[row_idx][col_idx];
}

// 读取 WAV 文件（仅支持 16-bit PCM 单声道）
static int16_t* read_wav(const char *filename, int *sample_rate, int *total_samples) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    
    char chunk_id[5] = {0};
    fread(chunk_id, 4, 1, f);
    if (strcmp(chunk_id, "RIFF") != 0) { fclose(f); return NULL; }
    fseek(f, 4, SEEK_CUR); // 跳过文件大小
    fread(chunk_id, 4, 1, f);
    if (strcmp(chunk_id, "WAVE") != 0) { fclose(f); return NULL; }
    
    int found_fmt = 0;
    while (!found_fmt) {
        fread(chunk_id, 4, 1, f);
        if (feof(f)) break;
        int chunk_size;
        fread(&chunk_size, 4, 1, f);
        if (strcmp(chunk_id, "fmt ") == 0) {
            found_fmt = 1;
            short audio_format, num_channels;
            fread(&audio_format, 2, 1, f);
            fread(&num_channels, 2, 1, f);
            fread(sample_rate, 4, 1, f);
            int byte_rate;
            fread(&byte_rate, 4, 1, f);
            short block_align, bits_per_sample;
            fread(&block_align, 2, 1, f);
            fread(&bits_per_sample, 2, 1, f);
            // 仅支持 PCM 16-bit 单声道
            if (audio_format != 1 || num_channels != 1 || bits_per_sample != 16) {
                fclose(f);
                return NULL;
            }
            // 跳过可能的扩展信息
            if (chunk_size > 16) fseek(f, chunk_size - 16, SEEK_CUR);
        } else {
            fseek(f, chunk_size, SEEK_CUR);
        }
    }
    if (!found_fmt) { fclose(f); return NULL; }
    
    // 查找 data chunk
    int found_data = 0;
    while (!found_data) {
        fread(chunk_id, 4, 1, f);
        if (feof(f)) break;
        int chunk_size;
        fread(&chunk_size, 4, 1, f);
        if (strcmp(chunk_id, "data") == 0) {
            found_data = 1;
            *total_samples = chunk_size / 2;
            int16_t *data = (int16_t*)malloc(chunk_size);
            if (!data) { fclose(f); return NULL; }
            fread(data, 1, chunk_size, f);
            fclose(f);
            return data;
        } else {
            fseek(f, chunk_size, SEEK_CUR);
        }
    }
    fclose(f);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    
    int sample_rate = 0, total_samples = 0;
    int16_t *samples = read_wav(argv[1], &sample_rate, &total_samples);
    if (!samples) return 1;
    
    // 设置帧长：约 25~30 毫秒，保证频率分辨率足够
    int frame_len = sample_rate / 40;   // 25ms
    if (frame_len < 120) frame_len = 120;
    if (frame_len > 400) frame_len = 400;
    int step = frame_len / 2;           // 50% 重叠
    

    char current_char = 0;
    int current_cnt = 0;
    int ready = 1; // 允许输出新字符
    char result[256] = {0};
    int pos = 0;

    for (int start = 0; start + frame_len <= total_samples; start += step) {
        char c = detect_frame(samples + start, frame_len, sample_rate);
        if (c != 0) {
            if (c == current_char) {
                current_cnt++;
            } else {
                current_char = c;
                current_cnt = 1;
            }
            if (ready && current_cnt == 3) {
                // 检测到新字符，允许输出
                result[pos++] = current_char;
                ready = 0; // 直到遇到静音/无效帧才允许再次输出
            }
        } else {
            current_char = 0;
            current_cnt = 0;
            ready = 1; // 静音/无效帧后允许输出
        }
    }
    
    printf("%s\n", result);
    free(samples);
    return 0;
}
