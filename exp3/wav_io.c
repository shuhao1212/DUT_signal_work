// wav_io.c
#include "wav_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_wav_float(const char* filename, float** data, int* sample_rate) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("无法打开文件: %s\n", filename);
        return -1;
    }
    
    WavHeader header;
    fread(&header, sizeof(WavHeader), 1, fp);
    
    // 验证格式
    if (strncmp(header.chunk_id, "RIFF", 4) != 0 ||
        strncmp(header.format, "WAVE", 4) != 0 ||
        header.audio_format != 1) {
        printf("非标准PCM WAV文件\n");
        fclose(fp);
        return -1;
    }
    
    *sample_rate = header.sample_rate;
    int num_samples = header.subchunk2_size / (header.bits_per_sample / 8);
    
    // 读取16位原始数据
    int16_t* raw_data = (int16_t*)malloc(num_samples * sizeof(int16_t));
    fread(raw_data, sizeof(int16_t), num_samples, fp);
    fclose(fp);
    
    // 转换为float
    *data = (float*)malloc(num_samples * sizeof(float));
    for (int i = 0; i < num_samples; i++) {
        (*data)[i] = raw_data[i] / 32768.0f;
    }
    
    free(raw_data);
    return num_samples;
}

int write_wav_float(const char* filename, const float* data, int num_samples, int sample_rate) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) return -1;
    
    WavHeader header;
    memcpy(header.chunk_id, "RIFF", 4);
    header.chunk_size = 36 + num_samples * 2;
    memcpy(header.format, "WAVE", 4);
    memcpy(header.subchunk1_id, "fmt ", 4);
    header.subchunk1_size = 16;
    header.audio_format = 1;
    header.num_channels = 1;
    header.sample_rate = sample_rate;
    header.bits_per_sample = 16;
    header.byte_rate = sample_rate * 1 * 2;
    header.block_align = 1 * 2;
    memcpy(header.subchunk2_id, "data", 4);
    header.subchunk2_size = num_samples * 2;
    
    fwrite(&header, sizeof(WavHeader), 1, fp);
    
    // 转换回16位并写入
    for (int i = 0; i < num_samples; i++) {
        float sample = data[i];
        // 限幅
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        int16_t raw_sample = (int16_t)(sample * 32767.0f);
        fwrite(&raw_sample, sizeof(int16_t), 1, fp);
    }
    
    fclose(fp);
    return 0;
}