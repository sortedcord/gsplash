#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <stdbool.h>

typedef struct AudioPlayer {
  AVFormatContext* format_ctx;
  AVCodecContext* codec_ctx;
  SwrContext* swr_ctx;
  AVFrame* frame;
  int stream_index;
  int sample_rate;
  int channels;
  SDL_AudioDeviceID audio_device;
  SDL_AudioSpec desired_spec;
  SDL_AudioSpec obtained_spec;
  uint8_t* audio_buffer;
  int audio_buffer_size;
  int audio_buffer_pos;
  int audio_buffer_filled;
} AudioPlayer;

bool init_audio_player(AudioPlayer* player, const char* path);
void cleanup_audio_player(AudioPlayer* player);
int decode_audio_frame(AudioPlayer* player, uint8_t* out_buffer, int out_size);
void audio_callback(void* userdata, uint8_t* stream, int len);

#endif  // AUDIO_H
