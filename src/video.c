#include "video.h"

#include <ctype.h>
#include <string.h>

bool has_video_extension(const char* path) {
  /*
  Check if the file extension of the given path matches common video formats.
  */

  // TODO: Instead of checking file extension, use MIME type detection

  // Check if the given path has an extension by looking for the last dot
  // character.
  const char* dot = strrchr(path, '.');
  if (!dot || dot == path) {
    return false;
  }

  char ext[8];
  size_t i = 0;
  dot++;
  while (*dot && i < sizeof(ext) - 1) {
    ext[i++] = (char)tolower((unsigned char)*dot++);
  }
  ext[i] = '\0';

  return strcmp(ext, "mp4") == 0 || strcmp(ext, "mkv") == 0 ||
         strcmp(ext, "webm") == 0 || strcmp(ext, "avi") == 0 ||
         strcmp(ext, "mov") == 0 || strcmp(ext, "mpg") == 0 ||
         strcmp(ext, "mpeg") == 0;
}

bool init_video_player(VideoPlayer* player, SDL_Renderer* renderer,
                       const char* path) {
  memset(player, 0, sizeof(*player));

  if (avformat_open_input(&player->format_ctx, path, NULL, NULL) != 0) {
    goto error;
  }

  if (avformat_find_stream_info(player->format_ctx, NULL) < 0) {
    goto error;
  }

  player->stream_index = -1;
  for (unsigned int i = 0; i < player->format_ctx->nb_streams; ++i) {
    if (player->format_ctx->streams[i]->codecpar->codec_type ==
        AVMEDIA_TYPE_VIDEO) {
      player->stream_index = (int)i;
      break;
    }
  }

  if (player->stream_index < 0) {
    goto error;
  }

  AVCodecParameters* codecpar =
      player->format_ctx->streams[player->stream_index]->codecpar;
  const AVCodec* decoder = avcodec_find_decoder(codecpar->codec_id);
  if (!decoder) {
    goto error;
  }

  player->codec_ctx = avcodec_alloc_context3(decoder);
  if (!player->codec_ctx) {
    goto error;
  }

  if (avcodec_parameters_to_context(player->codec_ctx, codecpar) < 0) {
    goto error;
  }

  if (avcodec_open2(player->codec_ctx, decoder, NULL) < 0) {
    goto error;
  }

  player->width = player->codec_ctx->width;
  player->height = player->codec_ctx->height;
  player->frame = av_frame_alloc();
  player->rgba_frame = av_frame_alloc();
  if (!player->frame || !player->rgba_frame) {
    goto error;
  }

  player->rgba_buffer_size = av_image_get_buffer_size(
      AV_PIX_FMT_RGBA, player->width, player->height, 1);
  player->rgba_buffer = (uint8_t*)av_malloc(player->rgba_buffer_size);
  if (!player->rgba_buffer) {
    goto error;
  }

  av_image_fill_arrays(player->rgba_frame->data, player->rgba_frame->linesize,
                       player->rgba_buffer, AV_PIX_FMT_RGBA, player->width,
                       player->height, 1);

  player->sws_ctx = sws_getContext(
      player->width, player->height, player->codec_ctx->pix_fmt, player->width,
      player->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
  if (!player->sws_ctx) {
    goto error;
  }

  player->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      player->width, player->height);
  if (!player->texture) {
    goto error;
  }

  AVRational fr =
      player->format_ctx->streams[player->stream_index]->avg_frame_rate;
  if (fr.num > 0 && fr.den > 0) {
    player->frame_delay_ms = (int)(1000.0 * fr.den / fr.num);
  } else {
    player->frame_delay_ms = 33;
  }

  player->next_frame_tick = SDL_GetTicks();
  return true;

error:
  cleanup_video_player(player);
  return false;
}

void cleanup_video_player(VideoPlayer* player) {
  if (player->texture) {
    SDL_DestroyTexture(player->texture);
  }
  if (player->sws_ctx) {
    sws_freeContext(player->sws_ctx);
  }
  if (player->rgba_buffer) {
    av_free(player->rgba_buffer);
  }
  if (player->rgba_frame) {
    av_frame_free(&player->rgba_frame);
  }
  if (player->frame) {
    av_frame_free(&player->frame);
  }
  if (player->codec_ctx) {
    avcodec_free_context(&player->codec_ctx);
  }
  if (player->format_ctx) {
    avformat_close_input(&player->format_ctx);
  }
  memset(player, 0, sizeof(*player));
}

bool decode_next_frame(VideoPlayer* player) {
  AVPacket* packet = av_packet_alloc();
  if (!packet) {
    return false;
  }

  while (av_read_frame(player->format_ctx, packet) >= 0) {
    if (packet->stream_index != player->stream_index) {
      av_packet_unref(packet);
      continue;
    }

    if (avcodec_send_packet(player->codec_ctx, packet) < 0) {
      av_packet_unref(packet);
      continue;
    }
    av_packet_unref(packet);

    while (true) {
      int ret = avcodec_receive_frame(player->codec_ctx, player->frame);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        break;
      }
      if (ret < 0) {
        av_packet_free(&packet);
        return false;
      }

      sws_scale(player->sws_ctx, (const uint8_t* const*)player->frame->data,
                player->frame->linesize, 0, player->height,
                player->rgba_frame->data, player->rgba_frame->linesize);

      SDL_UpdateTexture(player->texture, NULL, player->rgba_frame->data[0],
                        player->rgba_frame->linesize[0]);
      av_packet_free(&packet);
      return true;
    }
  }

  av_seek_frame(player->format_ctx, player->stream_index, 0,
                AVSEEK_FLAG_BACKWARD);
  avcodec_flush_buffers(player->codec_ctx);
  av_packet_free(&packet);
  return false;
}
