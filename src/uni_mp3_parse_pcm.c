/**********************************************************************
 * Copyright (C) 2017-2018  Unisound
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **********************************************************************
 *
 * Description : uni_mp3_parse_pcm.c
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.08.13
 *
 **********************************************************************/

#include "uni_log.h"
#include "uni_iot.h"
#include "uni_types.h"
#include "uni_mp3_parse_pcm.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"

#define MP3_PARSE_PCM_TAG    "mp3_parse_pcm"
#define MAX_AUDIO_FRAME_SIZE (40680)

typedef enum {
  BLOCK_NULL = 0,
  BLOCK_OPEN_STREAM,
  BLOCK_READ_HEADER,
  BLOCK_READ_FRAME,
} BlockState;

typedef struct _ConvertCtxNode{
  uni_u32                channel_layout;
  enum AVSampleFormat    sample_fmt;
  uni_s32                sample_rate;
  struct SwrContext      *au_convert_ctx;
  struct _ConvertCtxNode *next;
} ConvertCtxNode;

typedef struct {
  AVFormatContext     *pFormatCtx;
  AVCodecContext      *pCodecCtx;
  struct SwrContext   *au_convert_ctx;
  ConvertCtxNode      *convert_ctx_list;
  AVPacket            *packet;
  AVFrame             *pFrame;
  uni_s64             out_channel_layout;
  uni_s32             out_sample_rate;
  enum AVSampleFormat out_sample_fmt;
  uni_s32             out_channels;
  uni_s32             audioStream;
  uni_s32             out_buffer_size;
  uint8_t             *out_buffer;
  uni_s32             last_timestamp;
  uni_s32             block_state;
  uni_bool            initialize;
} Mp3ParsePcmInfo;

static Mp3ParsePcmInfo g_mp3_parse_pcm = {0};

static uni_s32 interrupt_cb(void *ctx) {
  uni_s32 seconds;
  uni_s32 timeout;
  seconds = time((time_t *)NULL);
  switch (g_mp3_parse_pcm.block_state) {
    case BLOCK_NULL:
      return 0;
    case BLOCK_OPEN_STREAM:
      timeout = 2;
      break;
    case BLOCK_READ_HEADER:
      timeout = 4;
      break;
    case BLOCK_READ_FRAME:
      timeout = 5;
      break;
    default:
      LOGW(MP3_PARSE_PCM_TAG, "invalid block state %d",
           g_mp3_parse_pcm.block_state);
      return 0;
  }
  if (seconds - g_mp3_parse_pcm.last_timestamp >= timeout) {
    LOGE(MP3_PARSE_PCM_TAG, "ffmpeg hit timeout at state %d!!!",
         g_mp3_parse_pcm.block_state);
    return 1;
  }
  return 0;
}

static const AVIOInterruptCB int_cb = {interrupt_cb, NULL, NULL,
                                       NULL, NULL, NULL, NULL};

static ConvertCtxNode* _create_convert_ctx_node(uni_u32 channel_layout,
                                                enum AVSampleFormat sample_fmt,
                                                uni_s32 sample_rate) {
  ConvertCtxNode *node, *head = g_mp3_parse_pcm.convert_ctx_list;
  node = uni_malloc(sizeof(ConvertCtxNode));
  node->channel_layout = channel_layout;
  node->sample_fmt = sample_fmt;
  node->sample_rate = sample_rate;
  node->au_convert_ctx = swr_alloc();
  node->au_convert_ctx = swr_alloc_set_opts(node->au_convert_ctx,
                                            g_mp3_parse_pcm.out_channel_layout,
                                            g_mp3_parse_pcm.out_sample_fmt,
                                            g_mp3_parse_pcm.out_sample_rate,
                                            node->channel_layout,
                                            node->sample_fmt,
                                            node->sample_rate,
                                            0,
                                            NULL);
  node->next = NULL;
  swr_init(node->au_convert_ctx);
  if (NULL == head) {
    g_mp3_parse_pcm.convert_ctx_list = node;
    return node;
  }
  while (NULL != head->next) {
    head = head->next;
  }
  head->next = node;
  return node;
}

static void _choose_au_convert_ctx(uni_u32 channel_layout,
                                   enum AVSampleFormat sample_fmt,
                                   uni_s32 sample_rate) {
  ConvertCtxNode *node = g_mp3_parse_pcm.convert_ctx_list;
  while (NULL != node) {
    if (node->channel_layout == channel_layout &&
        node->sample_fmt == sample_fmt &&
        node->sample_rate == sample_rate) {
      g_mp3_parse_pcm.au_convert_ctx = node->au_convert_ctx;
      return;
    }
    node = node->next;
  }
  node = _create_convert_ctx_node(channel_layout, sample_fmt, sample_rate);
  g_mp3_parse_pcm.au_convert_ctx = node->au_convert_ctx;
  return;
}

static Result _mp3_parse_prepare_internal(const char *url) {
  uni_u32 i;
  AVCodec *pCodec;
  uni_u32 in_channel_layout;
  uni_s32 in_nb_samples, out_nb_samples;
  uni_s32 tries = 3;
  g_mp3_parse_pcm.pFormatCtx = avformat_alloc_context();
  g_mp3_parse_pcm.block_state = BLOCK_OPEN_STREAM;
  g_mp3_parse_pcm.last_timestamp = time((time_t *)NULL);
  g_mp3_parse_pcm.pFormatCtx->interrupt_callback = int_cb;
  while (avformat_open_input(&g_mp3_parse_pcm.pFormatCtx,
                             url, NULL, NULL) != 0) {
    if (tries > 0) {
      LOGW(MP3_PARSE_PCM_TAG, "Open input stream times:%d", 3 - tries + 1);
      tries--;
    } else {
      LOGE(MP3_PARSE_PCM_TAG, "Couldn't open input stream");
      return E_FAILED;
    }
  }
  g_mp3_parse_pcm.block_state = BLOCK_READ_HEADER;
  g_mp3_parse_pcm.last_timestamp = time((time_t *)NULL);
  if (avformat_find_stream_info(g_mp3_parse_pcm.pFormatCtx, NULL) < 0) {
    LOGE(MP3_PARSE_PCM_TAG, "Couldn't find stream information");
    return E_FAILED;
  }
  g_mp3_parse_pcm.block_state = BLOCK_NULL;
  g_mp3_parse_pcm.audioStream = -1;
  for (i = 0; i < g_mp3_parse_pcm.pFormatCtx->nb_streams; i++) {
    if (g_mp3_parse_pcm.pFormatCtx->streams[i]->codec->codec_type ==
        AVMEDIA_TYPE_AUDIO) {
      g_mp3_parse_pcm.audioStream = i;
      break;
    }
  }
  if (g_mp3_parse_pcm.audioStream == -1) {
    LOGE(MP3_PARSE_PCM_TAG, "Didn't find a audio stream");
    return E_FAILED;
  }
  g_mp3_parse_pcm.pCodecCtx =
    g_mp3_parse_pcm.pFormatCtx->streams[g_mp3_parse_pcm.audioStream]->codec;
  pCodec = avcodec_find_decoder(g_mp3_parse_pcm.pCodecCtx->codec_id);
  if (pCodec == NULL) {
    LOGE(MP3_PARSE_PCM_TAG, "Codec not found");
    return E_FAILED;
  }
  if (avcodec_open2(g_mp3_parse_pcm.pCodecCtx, pCodec, NULL) < 0) {
    LOGE(MP3_PARSE_PCM_TAG, "Could not open codec");
    return E_FAILED;
  }
  in_nb_samples = g_mp3_parse_pcm.pCodecCtx->frame_size;
  out_nb_samples = in_nb_samples * g_mp3_parse_pcm.out_sample_rate /
                   g_mp3_parse_pcm.pCodecCtx->sample_rate + 1;
  LOGT(MP3_PARSE_PCM_TAG, "\nin_nb_samples is [%d]\nout_nb_samples is [%d]\n"
       "in_sample_rate is [%d]\nout_sample_rate is [%d]\n"
       "out_sample_fmt is [%d]\nout_channels is [%d]\n",
       in_nb_samples, out_nb_samples, g_mp3_parse_pcm.pCodecCtx->sample_rate,
       g_mp3_parse_pcm.out_sample_rate, g_mp3_parse_pcm.out_sample_fmt,
       g_mp3_parse_pcm.out_channels);
  g_mp3_parse_pcm.out_buffer_size = av_samples_get_buffer_size(NULL,
      g_mp3_parse_pcm.out_channels, out_nb_samples,
      g_mp3_parse_pcm.out_sample_fmt, 1);
  LOGT(MP3_PARSE_PCM_TAG, "g_mp3_parse_pcm.out_buffer_size is [%d]",
       g_mp3_parse_pcm.out_buffer_size);
  g_mp3_parse_pcm.out_buffer = (uint8_t *)av_malloc(g_mp3_parse_pcm.out_buffer_size);
  in_channel_layout = av_get_default_channel_layout(
      g_mp3_parse_pcm.pCodecCtx->channels);
  _choose_au_convert_ctx(in_channel_layout,
                         g_mp3_parse_pcm.pCodecCtx->sample_fmt,
                         g_mp3_parse_pcm.pCodecCtx->sample_rate);
  g_mp3_parse_pcm.packet = (AVPacket *)av_malloc(sizeof(AVPacket));
  av_init_packet(g_mp3_parse_pcm.packet);
  g_mp3_parse_pcm.pFrame = avcodec_alloc_frame();
  return E_OK;
}

static void _decode_tsk(void *args) {
  uni_s32 got_picture;
  uni_s32 ret = 0;
  uni_s32 rc = 0;
  CbPoolPcmData cb_pool_pcm_data = (CbPoolPcmData)args;
  static uni_s32 audio4ErrorCnt = 0;
  while (TRUE) {
    ret = 0;
    rc = 0;
    g_mp3_parse_pcm.block_state = BLOCK_READ_FRAME;
    g_mp3_parse_pcm.last_timestamp = time((time_t *)NULL);
    if (av_read_frame(g_mp3_parse_pcm.pFormatCtx,
                      g_mp3_parse_pcm.packet) >= 0) {
      if (g_mp3_parse_pcm.packet->stream_index == g_mp3_parse_pcm.audioStream) {
        if ((rc = avcodec_decode_audio4(g_mp3_parse_pcm.pCodecCtx,
                g_mp3_parse_pcm.pFrame,
                &got_picture, g_mp3_parse_pcm.packet)) < 0) {
          LOGE(MP3_PARSE_PCM_TAG, "error in decoding audio frame, rc=%d", rc);
          audio4ErrorCnt++;
          if (audio4ErrorCnt <= 10) {
            continue;
          }
          audio4ErrorCnt = 0;
          cb_pool_pcm_data(NULL, 0, MP3_PARSE_FAILED);
          break;
        }
        if (got_picture > 0) {
          const uint8_t *data = *g_mp3_parse_pcm.pFrame->data;
          uni_s32 data_len;
          data_len = swr_convert(g_mp3_parse_pcm.au_convert_ctx,
              &g_mp3_parse_pcm.out_buffer,
              MAX_AUDIO_FRAME_SIZE, &data,
              g_mp3_parse_pcm.pFrame->nb_samples);
          data_len = data_len * g_mp3_parse_pcm.out_channels *
            av_get_bytes_per_sample(g_mp3_parse_pcm.out_sample_fmt);
          cb_pool_pcm_data((char *)g_mp3_parse_pcm.out_buffer, data_len,
                           MP3_PARSE_OK);
          ret += data_len;
        }
      }
      av_free_packet(g_mp3_parse_pcm.packet);
      continue;
    }
    LOGT(MP3_PARSE_PCM_TAG, "mp3 play finished");
    cb_pool_pcm_data(NULL, 0, MP3_PARSE_DONE);
    break;
  }
  if (NULL != g_mp3_parse_pcm.out_buffer) {
    av_free(g_mp3_parse_pcm.out_buffer);
    g_mp3_parse_pcm.out_buffer = NULL;
  }
  if (NULL != g_mp3_parse_pcm.pCodecCtx) {
    avcodec_close(g_mp3_parse_pcm.pCodecCtx);
    g_mp3_parse_pcm.pCodecCtx = NULL;
  }
  if (NULL != g_mp3_parse_pcm.pFormatCtx) {
    avformat_close_input(&g_mp3_parse_pcm.pFormatCtx);
    avformat_free_context(g_mp3_parse_pcm.pFormatCtx);
    g_mp3_parse_pcm.pFormatCtx = NULL;
  }
}

static uni_s32 _get_decode_pcm_data(CbPoolPcmData cb_pool_pcm_data) {
  struct thread_param tThreadParm;
  uni_pthread_t pid;
  uni_memset(&tThreadParm, 0, sizeof(tThreadParm));
  tThreadParm.stack_size = STACK_SMALL_SIZE;
  tThreadParm.th_priority = OS_PRIORITY_NORMAL;
  uni_strncpy(tThreadParm.task_name, "decode_task",
              sizeof(tThreadParm.task_name));
  uni_pthread_create(&pid, &tThreadParm, _decode_tsk, cb_pool_pcm_data);
  uni_pthread_detach(pid);
}

static void _mp3_parse_pcm_init() {
  if (g_mp3_parse_pcm.initialize) {
    return;
  }
  g_mp3_parse_pcm.initialize = TRUE;
  g_mp3_parse_pcm.out_channels = 1;
  g_mp3_parse_pcm.out_sample_rate = 16000;
  g_mp3_parse_pcm.out_channel_layout = AV_CH_LAYOUT_MONO;
  g_mp3_parse_pcm.out_sample_fmt = AV_SAMPLE_FMT_S16;
  av_register_all();
  avformat_network_init();
  _create_convert_ctx_node(AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100);
}

int Mp3ParsePcm(const char *mp3_file_path, CbPoolPcmData cb_pool_pcm_data) {
  if (NULL == mp3_file_path || NULL == cb_pool_pcm_data) {
    LOGE(MP3_PARSE_PCM_TAG, "invalid param mp3_file_path=%p, "
         "cb_pool_pcm_data=%p", mp3_file_path, cb_pool_pcm_data);
    return -1;
  }
  _mp3_parse_pcm_init();
  if (E_OK != _mp3_parse_prepare_internal(mp3_file_path)) {
    LOGE(MP3_PARSE_PCM_TAG, "mp3 parse failed");
    return -1;
  }
  _get_decode_pcm_data(cb_pool_pcm_data);
  return 0;
}
