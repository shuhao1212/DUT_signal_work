// noise_reduction.h
#ifndef NOISE_REDUCTION_H
#define NOISE_REDUCTION_H

/**
 * 谱减法降噪
 * input       : 输入音频（float，[-1,1]）
 * input_len   : 输入长度（采样点）
 * sample_rate : 采样率（Hz）
 * output      : 输出降噪后音频（由调用者 free）
 * output_len  : 输出音频长度
 * return 0:成功，-1:内存不足
 */
int spectral_subtraction(const float* input, int input_len, 
                         int sample_rate, float** output, int* output_len);

#endif