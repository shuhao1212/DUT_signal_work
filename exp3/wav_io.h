// wav_io.h
#ifndef WAV_IO_H
#define WAV_IO_H

#include <stdint.h>

typedef struct {
    // RIFF头
    char     chunk_id[4];        // "RIFF"
    uint32_t chunk_size;         // 文件大小-8
    char     format[4];          // "WAVE"
    // fmt子块
    char     subchunk1_id[4];    // "fmt "
    uint32_t subchunk1_size;     // 16 for PCM
    uint16_t audio_format;       // 1 for PCM
    uint16_t num_channels;       // 1=单声道
    uint32_t sample_rate;        // 16000
    uint32_t byte_rate;          // sample_rate * num_channels * bits_per_sample/8
    uint16_t block_align;        // num_channels * bits_per_sample/8
    uint16_t bits_per_sample;    // 16
    // data子块
    char     subchunk2_id[4];    // "data"
    uint32_t subchunk2_size;     // 数据字节数
} WavHeader;

/**
 * 读取16位PCM WAV文件，转换为float数组（范围[-1,1]）
 * @return 读取的采样点数，-1表示失败
 */
int read_wav_float(const char* filename, float** data, int* sample_rate);

/**
 * 将float数组写入16位PCM WAV文件
 * @return 0:成功, -1:失败
 */
int write_wav_float(const char* filename, const float* data, int num_samples, int sample_rate);

#endif