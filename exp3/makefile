CC = gcc
CFLAGS = -Wall -O2 -std=c99
LDFLAGS = -lm

OBJS = main.o wav_io.o speed_change.o noise_reduction.o fft.o
TARGET = voice_speed

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)