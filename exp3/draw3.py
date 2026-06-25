import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile
from scipy.signal import spectrogram

# 读取两个 WAV
sr1, x = wavfile.read('input.wav')
sr2, y = wavfile.read('output.wav')

# 1. 波形对比
plt.figure()
plt.subplot(2,1,1)
plt.plot(x)
plt.title('Original')
plt.subplot(2,1,2)
plt.plot(y)
plt.title('Speed-changed')
plt.tight_layout()
plt.savefig('waveform_compare.png')

# 2. 语谱图对比
f1, t1, Sxx1 = spectrogram(x, fs=sr1)
f2, t2, Sxx2 = spectrogram(y, fs=sr2)
plt.figure()
plt.subplot(2,1,1)
plt.pcolormesh(t1, f1, 10*np.log10(Sxx1))
plt.title('Original Spectrogram')
plt.subplot(2,1,2)
plt.pcolormesh(t2, f2, 10*np.log10(Sxx2))
plt.title('Changed Spectrogram')
plt.tight_layout()
plt.savefig('spectrogram_compare.png')

# 3. 简单基频分析（用相关法）
def extract_f0(signal, sr, frame_size=512, hop=256):
    # 粗略估计，实际可用更成熟的librosa库
    f0s = []
    for i in range(0, len(signal)-frame_size, hop):
        frame = signal[i:i+frame_size].astype(np.float64)
        # 自相关法
        corr = np.correlate(frame, frame, mode='full')
        corr = corr[len(corr)//2:]
        # 找第一个峰值
        peaks = (corr[1:] > corr[:-1]) & (corr[1:] > corr[1:])  # 简化
        # 实际此处可安全用 librosa
        # 这里略过，建议用 librosa.pyin
    return np.array(f0s)

# 推荐用 librosa 做准确的基频提取
import librosa
f0_orig, _, _ = librosa.pyin(x.astype(float), fmin=50, fmax=500, sr=sr1)
f0_changed, _, _ = librosa.pyin(y.astype(float), fmin=50, fmax=500, sr=sr2)

print(f"Original mean F0: {np.nanmean(f0_orig):.2f} Hz")
print(f"Changed mean F0: {np.nanmean(f0_changed):.2f} Hz")