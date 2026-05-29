#ifndef VIDEO_H
#define VIDEO_H

#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdbool.h>

typedef struct VideoPlayer {
  AVFormatContext* format_ctx;
  AVCodecContext* codec_ctx;
  struct SwsContext* sws_ctx;
  AVFrame* frame;
  AVFrame* rgba_frame;
  uint8_t* rgba_buffer;
  int rgba_buffer_size;
  int stream_index;
  int audio_stream_index;
  int width;
  int height;
  int frame_delay_ms;
  Uint32 next_frame_tick;
  SDL_Texture* texture;
} VideoPlayer;

bool has_video_extension(const char* path);
bool init_video_player(VideoPlayer* player, SDL_Renderer* renderer,
                       const char* path);
void cleanup_video_player(VideoPlayer* player);
bool decode_next_frame(VideoPlayer* player);

#endif  // VIDEO_H
