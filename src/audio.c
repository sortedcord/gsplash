#include "audio.h"

#include <string.h>
#include <libavutil/opt.h>

void audio_callback(void* userdata, uint8_t* stream, int len) {
  AudioPlayer* player = (AudioPlayer*)userdata;
  if (!player) {
    memset(stream, 0, len);
    return;
  }

  int bytes_written = 0;
  while (bytes_written < len) {
    int remaining = len - bytes_written;

    if (player->audio_buffer_pos >= player->audio_buffer_filled) {
      int bytes_decoded = decode_audio_frame(player, player->audio_buffer,
                                             player->audio_buffer_size);
      if (bytes_decoded <= 0) {
        memset(stream + bytes_written, 0, remaining);
        return;
      }
      player->audio_buffer_pos = 0;
      player->audio_buffer_filled = bytes_decoded;
    }

    int available = player->audio_buffer_filled - player->audio_buffer_pos;
    int to_copy = (remaining < available) ? remaining : available;

    memcpy(stream + bytes_written,
           player->audio_buffer + player->audio_buffer_pos, to_copy);
    bytes_written += to_copy;
    player->audio_buffer_pos += to_copy;
  }
}

bool init_audio_player(AudioPlayer* player, const char* path) {
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
        AVMEDIA_TYPE_AUDIO) {
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

  player->sample_rate = player->codec_ctx->sample_rate;
  player->channels = player->codec_ctx->ch_layout.nb_channels;

  player->frame = av_frame_alloc();
  if (!player->frame) {
    goto error;
  }

  player->swr_ctx = swr_alloc();
  if (!player->swr_ctx) {
    goto error;
  }

  av_opt_set_chlayout(player->swr_ctx, "in_chlayout",
                      &player->codec_ctx->ch_layout, 0);
  av_opt_set_chlayout(player->swr_ctx, "out_chlayout",
                      &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO, 0);
  av_opt_set_int(player->swr_ctx, "in_sample_rate", player->sample_rate, 0);
  av_opt_set_int(player->swr_ctx, "out_sample_rate", player->sample_rate, 0);
  av_opt_set_sample_fmt(player->swr_ctx, "in_sample_fmt",
                        player->codec_ctx->sample_fmt, 0);
  av_opt_set_sample_fmt(player->swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16,
                        0);

  if (swr_init(player->swr_ctx) < 0) {
    goto error;
  }

  player->audio_buffer_size = player->sample_rate * player->channels * 2 *
                              2;
  player->audio_buffer = (uint8_t*)av_malloc(player->audio_buffer_size);
  if (!player->audio_buffer) {
    goto error;
  }

  SDL_zero(player->desired_spec);
  player->desired_spec.freq = player->sample_rate;
  player->desired_spec.format = AUDIO_S16;
  player->desired_spec.channels = 2;
  player->desired_spec.samples = 2048;
  player->desired_spec.callback = audio_callback;
  player->desired_spec.userdata = player;

  player->audio_device = SDL_OpenAudioDevice(NULL, 0, &player->desired_spec,
                                             &player->obtained_spec, 0);
  if (player->audio_device == 0) {
    goto error;
  }

  SDL_PauseAudioDevice(player->audio_device, 0);
  return true;

error:
  cleanup_audio_player(player);
  return false;
}

void cleanup_audio_player(AudioPlayer* player) {
  if (player->audio_device != 0) {
    SDL_CloseAudioDevice(player->audio_device);
  }
  if (player->audio_buffer) {
    av_free(player->audio_buffer);
  }
  if (player->swr_ctx) {
    swr_free(&player->swr_ctx);
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

int decode_audio_frame(AudioPlayer* player, uint8_t* out_buffer,
                       int out_size) {
  AVPacket* packet = av_packet_alloc();
  if (!packet) {
    return -1;
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
        return -1;
      }

      uint8_t* out[] = {out_buffer};
      int samples = swr_convert(player->swr_ctx, out, out_size / 4,
                                (const uint8_t**)player->frame->data,
                                player->frame->nb_samples);

      if (samples > 0) {
        int bytes_written = samples * 4;
        av_packet_free(&packet);
        return bytes_written;
      }
    }
  }

  av_seek_frame(player->format_ctx, player->stream_index, 0,
                AVSEEK_FLAG_BACKWARD);
  avcodec_flush_buffers(player->codec_ctx);
  av_packet_free(&packet);
  return -1;
}
