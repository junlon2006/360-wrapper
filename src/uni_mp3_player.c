/**************************************************************************
 * Copyright (C) 2017-2017  Unisound
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
 **************************************************************************
 *
 * Description : uni_mp3_player.c
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#include "uni_mp3_player.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "uni_log.h"

#define MP3_PLAYER_TAG       "mp3_player"
#define MAX_AUDIO_FRAME_SIZE 40680 // 1 second of 48khz 32bit audio

#define CAPTURE_PCM          0
#if CAPTURE_PCM
static uni_s32 f_pcm_out;
#endif

typedef enum {
  MP3_IDLE_STATE,
  MP3_PREPARING_STATE,
  MP3_PREPARED_STATE,
  MP3_PAUSED_STATE,
  MP3_PLAYING_STATE
} Mp3State;

typedef enum {
  MP3_PLAY_EVENT,
  MP3_PREPARE_EVENT,
  MP3_START_EVENT,
  MP3_PAUSE_EVENT,
  MP3_RESUME_EVENT,
  MP3_STOP_EVENT
} Mp3Event;

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

static struct {
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
  Mp3State            state;
  uni_pthread_t       prepare_thread;
  uni_mutex_t         mutex;
  uni_s64             time_base;
  uni_s32             last_timestamp;
  uni_s32             block_state;
} g_mp3_player = {NULL, NULL, NULL, NULL, NULL, NULL,
                  0, 0, 0, 0,
                  0, 0, NULL, MP3_IDLE_STATE, NULL, NULL,
                  0, BLOCK_NULL};

static uni_s32 interrupt_cb(void *ctx) {
  uni_s32 seconds;
  uni_s32 timeout;
  seconds = time((time_t *)NULL);
  switch (g_mp3_player.block_state) {
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
      LOGW(MP3_PLAYER_TAG, "invalid block state %d!!!!!!",
           g_mp3_player.block_state);
      return 0;
  }
  if (seconds - g_mp3_player.last_timestamp >= timeout) {
    LOGE(MP3_PLAYER_TAG, "ffmpeg hit timeout at state %d!!!",
         g_mp3_player.block_state);
    return 1;
  }
  return 0;
}

static const AVIOInterruptCB int_cb = {interrupt_cb, NULL, NULL,
                                       NULL, NULL, NULL, NULL};

static char* _event2string(Mp3Event event) {
  switch (event) {
  case MP3_PLAY_EVENT:
    return "MP3_PLAY_EVENT";
  case MP3_PREPARE_EVENT:
    return "MP3_PREPARE_EVENT";
  case MP3_START_EVENT:
    return "MP3_START_EVENT";
  case MP3_PAUSE_EVENT:
    return "MP3_PAUSE_EVENT";
  case MP3_RESUME_EVENT:
    return "MP3_RESUME_EVENT";
  case MP3_STOP_EVENT:
    return "MP3_STOP_EVENT";
  default:
    break;
  }
  return "N/A";
}

static char* _state2string(Mp3State state) {
  switch (state) {
    case MP3_IDLE_STATE:
      return "MP3_IDLE_STATE";
    case MP3_PREPARING_STATE:
      return "MP3_PREPARING_STATE";
    case MP3_PREPARED_STATE:
      return "MP3_PREPARED_STATE";
    case MP3_PAUSED_STATE:
      return "MP3_PAUSED_STATE";
    case MP3_PLAYING_STATE:
      return "MP3_PLAYING_STATE";
    default:
      break;
  }
  return "N/A";
}

static void _mp3_set_state(Mp3State state) {
  g_mp3_player.state = state;
  LOGT(MP3_PLAYER_TAG, "mp3 state is set to %d", state);
}

static ConvertCtxNode* _create_convert_ctx_node(uni_u32 channel_layout,
                                                enum AVSampleFormat sample_fmt,
                                                uni_s32 sample_rate) {
  ConvertCtxNode *node, *head = g_mp3_player.convert_ctx_list;
  node = uni_malloc(sizeof(ConvertCtxNode));
  node->channel_layout = channel_layout;
  node->sample_fmt = sample_fmt;
  node->sample_rate = sample_rate;
  node->au_convert_ctx = swr_alloc();
  node->au_convert_ctx = swr_alloc_set_opts(node->au_convert_ctx,
                                            g_mp3_player.out_channel_layout,
                                            g_mp3_player.out_sample_fmt,
                                            g_mp3_player.out_sample_rate,
                                            node->channel_layout,
                                            node->sample_fmt,
                                            node->sample_rate,
                                            0,
                                            NULL);
  node->next = NULL;
  swr_init(node->au_convert_ctx);
  if (NULL == head) {
    g_mp3_player.convert_ctx_list = node;
    return node;
  }
  while (NULL != head->next) {
    head = head->next;
  }
  head->next = node;
  return node;
}

static void _destroy_convert_ctx_node(ConvertCtxNode *node) {
  if (NULL != node) {
    swr_free(&node->au_convert_ctx);
    uni_free(node);
  }
}

static void _choose_au_convert_ctx(uni_u32 channel_layout,
                                   enum AVSampleFormat sample_fmt,
                                   uni_s32 sample_rate) {
  ConvertCtxNode *node = g_mp3_player.convert_ctx_list;
  while (NULL != node) {
    if (node->channel_layout == channel_layout &&
        node->sample_fmt == sample_fmt &&
        node->sample_rate == sample_rate) {
      g_mp3_player.au_convert_ctx = node->au_convert_ctx;
      return;
    }
    node = node->next;
  }
  node = _create_convert_ctx_node(channel_layout, sample_fmt, sample_rate);
  g_mp3_player.au_convert_ctx = node->au_convert_ctx;
  return;
}

static uni_s32 _audio_player_callback(DataBufHandle data_buffer) {
  uni_s32 got_picture;
  uni_s32 ret = 0;
  uni_s32 rc = 0;
  static uni_s32 audio4ErrorCnt = 0;
  uni_s64 byte_position = 0;
  if (MP3_PLAYING_STATE != g_mp3_player.state) {
    return 0;
  }
  g_mp3_player.block_state = BLOCK_READ_FRAME;
  g_mp3_player.last_timestamp = time((time_t *)NULL);
  byte_position = g_mp3_player.pFormatCtx->pb->pos;
  if (av_read_frame(g_mp3_player.pFormatCtx, g_mp3_player.packet) >= 0) {
    LOGW(MP3_PLAYER_TAG, "pos=%lld, ptr=%p, end=%p, offset=%d, size=%d, "
         "start_time=%lld, duration=%f, packet_size=%u, data_offset=%lld", byte_position,
         g_mp3_player.pFormatCtx->pb->buf_ptr,
         g_mp3_player.pFormatCtx->pb->buf_end,
         g_mp3_player.pFormatCtx->pb->buf_end -
         g_mp3_player.pFormatCtx->pb->buf_ptr,
         g_mp3_player.pFormatCtx->pb->buffer_size,
         g_mp3_player.pFormatCtx->start_time,
         g_mp3_player.pFormatCtx->duration,
         g_mp3_player.pFormatCtx->packet_size,
         g_mp3_player.pFormatCtx->data_offset);
    if (g_mp3_player.packet->stream_index == g_mp3_player.audioStream) {
      if ((rc = avcodec_decode_audio4(g_mp3_player.pCodecCtx, g_mp3_player.pFrame,
                                &got_picture, g_mp3_player.packet)) < 0) {
        LOGE(MP3_PLAYER_TAG, "Error in decoding audio frame, rc=%d", rc);
        audio4ErrorCnt++;
        return 0;
        if (audio4ErrorCnt <= 10) {
          return 0;
        }
        audio4ErrorCnt = 0;
        return -1;
      }
      if (got_picture > 0) {
        const uint8_t *data = *g_mp3_player.pFrame->data;
        uni_s32 data_len;
        data_len = swr_convert(g_mp3_player.au_convert_ctx,
                               &g_mp3_player.out_buffer,
                               MAX_AUDIO_FRAME_SIZE, &data,
                               g_mp3_player.pFrame->nb_samples);
        data_len = data_len * g_mp3_player.out_channels *
                   av_get_bytes_per_sample(g_mp3_player.out_sample_fmt);
        DataBufferWrite(data_buffer, (char *)g_mp3_player.out_buffer, data_len);
#if CAPTURE_PCM
        uni_hal_fwrite(f_pcm_out, (char *)g_mp3_player.out_buffer, data_len);
#endif
        ret += data_len;
      }
    }
    av_free_packet(g_mp3_player.packet);
  } else {
    LOGT(MP3_PLAYER_TAG, "mp3 play finished");
    return -1;
  }
  return ret;
}

static Result _mp3_prepare_internal(const char *url) {
  uni_u32 i;
  AVCodec *pCodec;
  uni_u32 in_channel_layout;
  //Out Audio Param
  uni_s32 in_nb_samples, out_nb_samples;
  /* in case not success the first time */
  uni_s32 tries = 3;
  av_register_all();
  avformat_network_init();
  g_mp3_player.pFormatCtx = avformat_alloc_context();
  g_mp3_player.block_state = BLOCK_OPEN_STREAM;
  g_mp3_player.last_timestamp = time((time_t *)NULL);
  g_mp3_player.pFormatCtx->interrupt_callback = int_cb;
  while (avformat_open_input(&g_mp3_player.pFormatCtx, url, NULL, NULL) != 0) {
    if (tries > 0) {
      LOGW(MP3_PLAYER_TAG, "Open input stream times:%d", 3 - tries + 1);
      tries--;
    } else {
      LOGE(MP3_PLAYER_TAG, "Couldn't open input stream");
      return E_FAILED;
    }
  }
  g_mp3_player.block_state = BLOCK_READ_HEADER;
  g_mp3_player.last_timestamp = time((time_t *)NULL);
  // Retrieve stream information, time consuming
  if (avformat_find_stream_info(g_mp3_player.pFormatCtx, NULL) < 0) {
    LOGE(MP3_PLAYER_TAG, "Couldn't find stream information");
    return E_FAILED;
  }
  g_mp3_player.block_state = BLOCK_NULL;
  // Dump valid information onto standard error
  //av_dump_format(g_mp3_player.pFormatCtx, 0, url, 0);
  // Find the first audio stream
  g_mp3_player.audioStream = -1;
  for (i = 0; i < g_mp3_player.pFormatCtx->nb_streams; i++) {
    if (g_mp3_player.pFormatCtx->streams[i]->codec->codec_type ==
        AVMEDIA_TYPE_AUDIO) {
      g_mp3_player.audioStream = i;
      break;
    }
  }
  if (g_mp3_player.audioStream == -1) {
    LOGE(MP3_PLAYER_TAG, "Didn't find a audio stream");
    return E_FAILED;
  }
  // Get a pointer to the codec context for the audio stream
  g_mp3_player.pCodecCtx =
    g_mp3_player.pFormatCtx->streams[g_mp3_player.audioStream]->codec;
  // Find the decoder for the audio stream
  pCodec = avcodec_find_decoder(g_mp3_player.pCodecCtx->codec_id);
  if (pCodec == NULL) {
    LOGE(MP3_PLAYER_TAG, "Codec not found");
    return E_FAILED;
  }
  // Open codec
  if (avcodec_open2(g_mp3_player.pCodecCtx, pCodec, NULL) < 0) {
    LOGE(MP3_PLAYER_TAG, "Could not open codec");
    return E_FAILED;
  }
  //nb_samples: AAC-1024 MP3-1152
  in_nb_samples = g_mp3_player.pCodecCtx->frame_size;
  out_nb_samples = in_nb_samples * g_mp3_player.out_sample_rate /
                   g_mp3_player.pCodecCtx->sample_rate + 1;
  LOGT(MP3_PLAYER_TAG, "\nin_nb_samples is [%d]\nout_nb_samples is [%d]\n"
       "in_sample_rate is [%d]\nout_sample_rate is [%d]\n"
       "out_sample_fmt is [%d]\nout_channels is [%d]\n",
       in_nb_samples, out_nb_samples, g_mp3_player.pCodecCtx->sample_rate,
       g_mp3_player.out_sample_rate, g_mp3_player.out_sample_fmt,
       g_mp3_player.out_channels);
  //Out Buffer Size
  g_mp3_player.time_base = ((uni_s64)g_mp3_player.pCodecCtx->time_base.num *
                           AV_TIME_BASE) / (uni_s64)(g_mp3_player.pCodecCtx->time_base.den);
  LOGE(MP3_PLAYER_TAG, "num=%lld, den=%lld, base=%lld", g_mp3_player.pCodecCtx->time_base.num,
             g_mp3_player.pCodecCtx->time_base.den, g_mp3_player.time_base);
  g_mp3_player.out_buffer_size = av_samples_get_buffer_size(NULL,
      g_mp3_player.out_channels,
      out_nb_samples, g_mp3_player.out_sample_fmt, 1);
  LOGT(MP3_PLAYER_TAG, "g_mp3_player.out_buffer_size is [%d]",
       g_mp3_player.out_buffer_size);
  g_mp3_player.out_buffer = (uint8_t *)av_malloc(g_mp3_player.out_buffer_size);
  //FIX:Some Codec's Context Information is missing
  in_channel_layout = av_get_default_channel_layout(
      g_mp3_player.pCodecCtx->channels);
  //Swr
  _choose_au_convert_ctx(in_channel_layout,
                         g_mp3_player.pCodecCtx->sample_fmt,
                         g_mp3_player.pCodecCtx->sample_rate);
  g_mp3_player.packet = (AVPacket *)av_malloc(sizeof(AVPacket));
  av_init_packet(g_mp3_player.packet);
  g_mp3_player.pFrame = avcodec_alloc_frame();
  return E_OK;
}

static void _mp3_start_internal(void) {
#if CAPTURE_PCM
  f_pcm_out = uni_hal_fopen("pcm_out.pcm", O_CREAT | O_RDWR);
#endif
  AudioPlayerStart(_audio_player_callback);
}

static Result _mp3_release_internal(void) {
  if (NULL != g_mp3_player.packet) {
    av_free(g_mp3_player.packet);
    g_mp3_player.packet = NULL;
  }
  if (NULL != g_mp3_player.pFrame) {
    avcodec_free_frame(&g_mp3_player.pFrame);
    g_mp3_player.pFrame= NULL;
  }
  g_mp3_player.au_convert_ctx= NULL;
  if (NULL != g_mp3_player.out_buffer) {
    av_free(g_mp3_player.out_buffer);
    g_mp3_player.out_buffer= NULL;
  }
  if (NULL != g_mp3_player.pCodecCtx) {
    avcodec_close(g_mp3_player.pCodecCtx);
    g_mp3_player.pCodecCtx= NULL;
  }
  if (NULL != g_mp3_player.pFormatCtx) {
    avformat_close_input(&g_mp3_player.pFormatCtx);
    avformat_free_context(g_mp3_player.pFormatCtx);
    g_mp3_player.pFormatCtx= NULL;
  }
  avformat_network_deinit();
  return E_OK;
}

static void _mp3_stop_internal(void) {
  AudioPlayerStop();
  uni_msleep(400);
#if CAPTURE_PCM
  uni_hal_fclose(f_pcm_out);
#endif
}

static void _mp3_prepare_routine(void *arg) {
  uni_pthread_detach(g_mp3_player.prepare_thread);
  uni_pthread_mutex_lock(g_mp3_player.mutex);
  if (E_OK != _mp3_prepare_internal((char *)arg)) {
    _mp3_set_state(MP3_IDLE_STATE);
    LOGE(MP3_PLAYER_TAG, "mp3 prepared failed");
  } else {
    _mp3_set_state(MP3_PREPARED_STATE);
    LOGT(MP3_PLAYER_TAG, "mp3 prepared succedded");
  }
  uni_pthread_mutex_unlock(g_mp3_player.mutex);
}

static Result _mp3_fsm(Mp3Event event, void *param) {
  Result rc = E_FAILED;
  uni_pthread_mutex_lock(g_mp3_player.mutex);
  switch (g_mp3_player.state) {
    case MP3_IDLE_STATE:
      if (MP3_PLAY_EVENT == event) {
        if (E_OK == _mp3_prepare_internal((char *)param)) {
          _mp3_start_internal();
          _mp3_set_state(MP3_PLAYING_STATE);
          rc = E_OK;
        }
      } else if (MP3_PREPARE_EVENT == event) {
        struct thread_param tThreadParm;
        uni_memset(&tThreadParm, 0, sizeof(tThreadParm));
        tThreadParm.stack_size = STACK_SMALL_SIZE;
        tThreadParm.th_priority = OS_PRIORITY_NORMAL;
        uni_strncpy(tThreadParm.task_name, "prepare_task",
                    sizeof(tThreadParm.task_name));
        uni_pthread_create(&g_mp3_player.prepare_thread,
            &tThreadParm, _mp3_prepare_routine, param);
        _mp3_set_state(MP3_PREPARING_STATE);
        rc = E_OK;
      }
      break;
    case MP3_PREPARING_STATE:
      if (MP3_START_EVENT == event || MP3_RESUME_EVENT == event) {
        while (MP3_PREPARING_STATE == g_mp3_player.state) {
          uni_msleep(100);
          LOGT(MP3_PLAYER_TAG, "waiting while mp3 is preparing");
        }
        if (MP3_PREPARED_STATE == g_mp3_player.state) {
          _mp3_start_internal();
          _mp3_set_state(MP3_PLAYING_STATE);
          rc = E_OK;
        }
      }
      break;
    case MP3_PREPARED_STATE:
      if (MP3_START_EVENT == event || MP3_RESUME_EVENT == event) {
        _mp3_start_internal();
        _mp3_set_state(MP3_PLAYING_STATE);
        rc = E_OK;
      } else if (MP3_STOP_EVENT == event) {
        _mp3_release_internal();
        _mp3_set_state(MP3_IDLE_STATE);
        rc = E_OK;
      }
      break;
    case MP3_PLAYING_STATE:
      if (MP3_PAUSE_EVENT == event) {
        _mp3_stop_internal();
        _mp3_set_state(MP3_PAUSED_STATE);
        rc = E_OK;
      } else if (MP3_STOP_EVENT == event) {
        _mp3_stop_internal();
        _mp3_release_internal();
        _mp3_set_state(MP3_IDLE_STATE);
        rc = E_OK;
      }
      break;
    case MP3_PAUSED_STATE:
      if (MP3_RESUME_EVENT == event) {
        _mp3_start_internal();
        _mp3_set_state(MP3_PLAYING_STATE);
        rc = E_OK;
      } else if (MP3_STOP_EVENT == event) {
        _mp3_release_internal();
        _mp3_set_state(MP3_IDLE_STATE);
        rc = E_OK;
      }
      break;
    default:
      break;
  }
  uni_pthread_mutex_unlock(g_mp3_player.mutex);
  LOGT(MP3_PLAYER_TAG, "event %s, state %s, result %s",
      _event2string(event), _state2string(g_mp3_player.state),
      rc == E_OK ? "OK" : "FAILED");
  return rc;
}

Result Mp3Play(char *filename) {
  return _mp3_fsm(MP3_PLAY_EVENT, (void *)filename);
}

Result Mp3Prepare(char *filename) {
  LOGT(MP3_PLAYER_TAG, "playing %s", filename);
  return _mp3_fsm(MP3_PREPARE_EVENT, (void *)filename);
}

Result Mp3Start(void) {
  return _mp3_fsm(MP3_START_EVENT, NULL);
}

Result Mp3Pause(void) {
  return _mp3_fsm(MP3_PAUSE_EVENT, NULL);
}

Result Mp3Resume(void) {
  return _mp3_fsm(MP3_RESUME_EVENT, NULL);
}

Result Mp3Stop(void) {
  return _mp3_fsm(MP3_STOP_EVENT, NULL);
}

Result Mp3Init(AudioParam *param) {
  uni_pthread_mutex_init(&g_mp3_player.mutex);
  g_mp3_player.out_channels = param->channels;
  g_mp3_player.out_sample_rate = param->rate;
  if (param->channels == 1) {
    g_mp3_player.out_channel_layout = AV_CH_LAYOUT_MONO;
  } else {
    g_mp3_player.out_channel_layout = AV_CH_LAYOUT_STEREO;
  }
  if (param->bit == 16) {
    g_mp3_player.out_sample_fmt = AV_SAMPLE_FMT_S16;
  } else {
    g_mp3_player.out_sample_fmt = AV_SAMPLE_FMT_S32;
  }
  /* put most often used configuration in list */
  _create_convert_ctx_node(AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100);
  return E_OK;
}

Result Mp3Final(void) {
  ConvertCtxNode *head = g_mp3_player.convert_ctx_list;
  ConvertCtxNode *node = head;
  uni_pthread_mutex_destroy(g_mp3_player.mutex);
  while (NULL != node) {
    head = node->next;
    _destroy_convert_ctx_node(node);
    node = head;
  }
  return E_OK;
}

Result Mp3SeekToMsec(int msec) {
  LOGW(MP3_PLAYER_TAG, "@@@@@@@@@@@@@@@@@@@time_base=%d",
       g_mp3_player.time_base);
#if 1
  double m_start_time = 100.0;
  int seek_ts = m_start_time * (g_mp3_player.pCodecCtx->time_base.den) / 
    g_mp3_player.pCodecCtx->time_base.num;
  av_seek_frame(g_mp3_player.pFormatCtx, g_mp3_player.audioStream,
                //g_mp3_player.pFormatCtx->duration/1000 * 0.1,
      seek_ts,
                AVSEEK_FLAG_ANY);
#endif
  //avio_seek(g_mp3_player.pFormatCtx->pb, 2523206, SEEK_SET);
}

uni_bool Mp3CheckIsPlaying(void) {
  return (g_mp3_player.state == MP3_PLAYING_STATE);
}

uni_bool Mp3CheckIsPause(void) {
  return (g_mp3_player.state == MP3_PAUSED_STATE);
}
