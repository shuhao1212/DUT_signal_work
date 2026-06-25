// speed_change.h
#ifndef SPEED_CHANGE_H
#define SPEED_CHANGE_H

/**
 * 语音变速不变调
 * @param input     输入音频数据（float，范围[-1, 1]）
 * @param input_len 输入音频长度（采样点数）
 * @param output    输出音频数据缓冲区
 * @param output_len 输出音频长度
 * @param p         分子（p/q为变速比，>1加速，<1减速）
 * @param q         分母
 * @param frame_len 帧长（建议256、512等，需为p、q的整数倍）
 * @return 0:成功, -1:内存错误
 */
int change_speed(const float* input, int input_len, float* output, int* output_len,
                 int p, int q, int frame_len);

#endif