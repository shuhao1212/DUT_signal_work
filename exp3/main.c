// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wav_io.h"
#include "speed_change.h"
#include "noise_reduction.h"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("用法: %s <输入WAV> <输出WAV> <倍速(如0.5,1.5,2.0)> [denoise]\n", argv[0]);
        printf("注：若输入文件名包含 white_noise 或指定 denoise 参数，将自动降噪\n");
        return 1;
    }
    
    const char* input_file  = argv[1];
    const char* output_file = argv[2];
    float target_speed = atof(argv[3]);
    int do_denoise = 0;
    if (argc >= 5 && strcmp(argv[4], "denoise") == 0) {
        do_denoise = 1;
    }
    if (strstr(input_file, "white_noise") != NULL) {
        do_denoise = 1;
    }
    
    // 倍速转分数 p/q
    int p, q;
    q = 100;
    p = (int)(target_speed * 100 + 0.5f);
    int a = p, b = q;
    while (b) { int t = b; b = a % b; a = t; }
    p /= a;
    q /= a;
    printf("变速倍速: %.2f = %d/%d\n", target_speed, p, q);
    
    // 1. 读取原始音频
    float* raw_data = NULL;
    int sample_rate = 0;
    int raw_len = read_wav_float(input_file, &raw_data, &sample_rate);
    if (raw_len <= 0) {
        printf("读取音频失败\n");
        return 1;
    }
    
    float* clean_data = NULL;
    int clean_len = raw_len;
    
    // 2. 判断是否需要降噪（文件名包含 white_noise 或指定 denoise 参数）
    if (do_denoise) {
        printf("开始谱减法降噪...\n");
        float* denoised = NULL;
        int denoised_len = 0;
        if (spectral_subtraction(raw_data, raw_len, sample_rate, 
                                 &denoised, &denoised_len) != 0) {
            printf("降噪失败\n");
            free(raw_data);
            return 1;
        }
        free(raw_data);
        clean_data = denoised;
        clean_len = denoised_len;
        printf("降噪完成，输出长度：%d 采样点\n", clean_len);
    } else {
        clean_data = raw_data; // 不用降噪，直接使用原始数据
    }
    
    // 3. 变速处理
    //   使用稳定的帧长和相位声码器，以保留语音清晰度
    int frame_len = 512;
    int max_output_len = (int)((double)clean_len * q / p) + frame_len;
    float* output_data = (float*)malloc(max_output_len * sizeof(float));
    if (!output_data) {
        free(clean_data);
        return 1;
    }
    
    int output_len = 0;
    if (change_speed(clean_data, clean_len, output_data, 
                     &output_len, p, q, frame_len) != 0) {
        printf("变速处理失败\n");
        free(clean_data);
        free(output_data);
        return 1;
    }

    // 4. 写回 WAV
    write_wav_float(output_file, output_data, output_len, sample_rate);
    printf("输出文件：%s，时长：%.2f 秒\n", 
           output_file, (float)output_len / sample_rate);
    
    free(clean_data);
    free(output_data);
    return 0;
}

// gcc -O2 -std=c99 -o voice_speed main.c wav_io.c speed_change.c noise_reduction.c fft.c -lm
// # 原始音频变速0.5倍（不降噪）
// ./voice_speed 20241034150.wav out_0.5.wav 0.5

// # 带白噪声音频自动降噪+变速0.5倍
//   只对白噪声文件去噪，因为文件名包含 white_noise
// ./voice_speed 20241034150_white_noise_20pct.wav out_noise_0.5.wav 0.5