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
 * Description : uni_audio_player.c
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/

#include "uni_audio_player.h"
#include "uni_log.h"

#define AUDIO_PLAYER_TAG         "aduio_player"
#define PCM_FRAME_SIZE           (640)
#define AUDIO_READ_PER_FRAME_CNT (5)
#define AUDIO_ALL_STOPPED        (-1)

static struct {
  DataBufHandle      databuf_handle[AUDIO_PLAYER_CNT];
  uni_bool           audio_player_playing[AUDIO_PLAYER_CNT];
  AudioPlayerType    cur_front_type;
  float              front_audio_ratio[AUDIO_PLAYER_CNT];
  AudioPlayerInputCb input_handler[AUDIO_PLAYER_CNT];
  void               *audioout_handler;
  uni_pthread_t      pid;
  uni_bool           running;
  uni_sem_t          wait_start;
  uni_sem_t          wait_stop;
  uni_sem_t          sem_thread_exit_sync;
  uni_mutex_t        mutex;
  uni_s32            volume;
} g_audio_player;

static void _reset_mix_status(AudioPlayerType type) {
  if (type == g_audio_player.cur_front_type) {
    g_audio_player.cur_front_type = AUDIO_NULL_PLAYER;
  }
}

static void _get_enough_source_data() {
  uni_s32 data_len;
  uni_s32 i;
  for (i = 0; i < AUDIO_PLAYER_CNT; i++) {
    if (!g_audio_player.audio_player_playing[i]) {
      continue;
    }
    data_len = DataBufferGetDataSize(g_audio_player.databuf_handle[i]);
    if (PCM_FRAME_SIZE < data_len) {
      continue;
    }
    if (-1 == g_audio_player.input_handler[i](
              g_audio_player.databuf_handle[i])) {
      g_audio_player.audio_player_playing[i] = FALSE;
      _reset_mix_status(i);
      LOGW(AUDIO_PLAYER_TAG, "audio input[%d] finished or failed", i);
    }
  }
}

static void _generate_mixed_data(char *output, uni_s32 len) {
  char buf[PCM_FRAME_SIZE] = {0};
  uni_s32 i, j;
  uni_s32 data_len;
  float ratio;
  short *out;
  short *in;
  memset(output, 0, PCM_FRAME_SIZE);
  for (i = 0; i < AUDIO_PLAYER_CNT; i++) {
    data_len = DataBufferGetDataSize(g_audio_player.databuf_handle[i]);
    if (data_len <= 0) {
      continue;
    }
    DataBufferRead(buf, uni_min(data_len, PCM_FRAME_SIZE),
                   g_audio_player.databuf_handle[i]);
    if (AUDIO_NULL_PLAYER == g_audio_player.cur_front_type) {
      memcpy(output, buf, PCM_FRAME_SIZE);
      break;
    }
    if (i == g_audio_player.cur_front_type) {
      ratio = g_audio_player.front_audio_ratio[g_audio_player.cur_front_type];
    } else {
      ratio = 1 -
        g_audio_player.front_audio_ratio[g_audio_player.cur_front_type];
    }
    out = (short *)output;
    in = (short *)buf;
    for (j = 0; j < uni_min(data_len, PCM_FRAME_SIZE) / 2; j++) {
      *out += (*in) * ratio;
      out++;
      in++;
    }
  }
}

static void _feed_buffer(char *buf, uni_s32 len) {
  uni_hal_audio_write(g_audio_player.audioout_handler, buf, PCM_FRAME_SIZE);
}

static uni_bool _is_all_player_stopped() {
  uni_s32 active_player_cnt = 0;
  uni_s32 i;
  uni_pthread_mutex_lock(g_audio_player.mutex);
  for (i = 0; i < AUDIO_PLAYER_CNT; i++) {
    if (g_audio_player.audio_player_playing[i]) {
      active_player_cnt++;
    }
  }
  uni_pthread_mutex_unlock(g_audio_player.mutex);
  if (0 == active_player_cnt) {
    LOGT(AUDIO_PLAYER_TAG, "all active player stopped");
  }
  return active_player_cnt == 0 ? TRUE : FALSE;
}

static uni_s32 _audio_write(uni_u32 frames) {
  char write_buf[PCM_FRAME_SIZE] = {0};
  uni_u32 total_frames = 0;
  uni_s32 i;
  uni_s32 rc;
  while (total_frames++ < frames) {
    _get_enough_source_data();
    _generate_mixed_data(write_buf, sizeof(write_buf));
    _feed_buffer(write_buf, sizeof(write_buf));
    if (_is_all_player_stopped()) {
      return AUDIO_ALL_STOPPED;
    }
  }
  return 0;
}

static void _player_start_internal(void) {
  uni_hal_audio_stop(g_audio_player.audioout_handler);
  uni_hal_audio_start(g_audio_player.audioout_handler);
}

static void _player_stop_internal(void) {
  int i;
  for (i = 0; i < AUDIO_PLAYER_CNT; i++) {
    DataBufferClear(g_audio_player.databuf_handle[i]);
  }
}

static void _audio_player_process(void *args) {
  g_audio_player.running = TRUE;
  while (g_audio_player.running) {
    uni_sem_wait(g_audio_player.wait_start);
    _player_start_internal();
    LOGT(AUDIO_PLAYER_TAG, "playing started!!!");
    while (TRUE) {
      if (AUDIO_ALL_STOPPED == _audio_write(AUDIO_READ_PER_FRAME_CNT)) {
        break;
      }
    }
    _player_stop_internal();
    LOGT(AUDIO_PLAYER_TAG, "playing finished!!!");
  }
  uni_sem_post(g_audio_player.sem_thread_exit_sync);
}

static Result _audioout_init(AudioPlayerParam *param) {
  struct hal_audio_config audio_config;
  audio_config.channels = param->channels;
  audio_config.bits = param->bit;
  audio_config.rate = param->rate;
  audio_config.period_size = 256;
  audio_config.period_count = 8;
  audio_config.is_play = 1;
  LOGT(AUDIO_PLAYER_TAG, "channels=%d, bit=%d, rate=%d, period_size=%d, "
       "period_count=%d.", param->channels, param->bit, param->rate,
       audio_config.period_size, audio_config.period_count);
  g_audio_player.audioout_handler = uni_hal_audio_open(&audio_config);
  if (NULL == g_audio_player.audioout_handler) {
    LOGE(AUDIO_PLAYER_TAG, "create audio out handler failed");
    return E_FAILED;
  }
  g_audio_player.volume = VOLUME_DEFAULT;
  uni_hal_audio_set_volume(g_audio_player.audioout_handler,
                           g_audio_player.volume);
  return E_OK;
}

static void _audioout_final(void) {
  uni_hal_audio_close(g_audio_player.audioout_handler);
}

static Result _create_audio_player_thread(void) {
  struct thread_param param;
  uni_memset(&param, 0, sizeof(param));
  param.stack_size = STACK_SMALL_SIZE;
  param.th_priority = OS_PRIORITY_NORMAL;
  uni_strncpy(param.task_name, "audio_player", sizeof(param.task_name));
  if (0 != uni_pthread_create(&g_audio_player.pid, &param,
                              _audio_player_process, NULL)) {
    LOGE(AUDIO_PLAYER_TAG, "create thread failed");
    return E_FAILED;
  }
  uni_pthread_detach(g_audio_player.pid);
  return E_OK;
}

static void _databuf_create() {
  int i;
  for (i = 0; i < AUDIO_PLAYER_CNT; i++) {
    g_audio_player.databuf_handle[i] = DataBufferCreate(4096);
  }
}

static void _mutex_init() {
  uni_pthread_mutex_init(&g_audio_player.mutex);
}

static void _sem_init() {
  uni_sem_init(&g_audio_player.wait_start, 0);
  uni_sem_init(&g_audio_player.wait_stop, 0);
  uni_sem_init(&g_audio_player.sem_thread_exit_sync, 0);
}

static void _databuf_destroy() {
  int i;
  for (i = 0; i < AUDIO_PLAYER_CNT; i++) {
    DataBufferDestroy(g_audio_player.databuf_handle[i]);
  }
}

static void _mutex_destroy() {
  uni_pthread_mutex_destroy(g_audio_player.mutex);
}

static void _sem_destroy() {
  uni_sem_destroy(g_audio_player.wait_start);
  uni_sem_destroy(g_audio_player.wait_stop);
  uni_sem_destroy(g_audio_player.sem_thread_exit_sync);
}

static void _set_front_audio_type(AudioPlayerType type) {
  g_audio_player.cur_front_type = type;
  LOGT(AUDIO_PLAYER_TAG, "type=%d", type);
}

Result AudioPlayerInit(AudioPlayerParam *param) {
  uni_memset(&g_audio_player, 0, sizeof(g_audio_player));
  _set_front_audio_type(AUDIO_NULL_PLAYER);
  _databuf_create();
  _mutex_init();
  _sem_init();
  if (E_OK != _audioout_init(param)) {
    goto L_AUDIOOUT_INIT_FAILED;
  }
  if (E_OK != _create_audio_player_thread()) {
    goto L_PTHREAD_CREATE_FAILED;
  }
  return E_OK;
L_PTHREAD_CREATE_FAILED:
  _audioout_final();
L_AUDIOOUT_INIT_FAILED:
  _databuf_destroy();
  _mutex_destroy();
  _sem_destroy();
  return E_FAILED;
}

static void _worker_thread_exit() {
  g_audio_player.running = FALSE;
  uni_sem_post(g_audio_player.wait_start);
  uni_sem_wait(g_audio_player.sem_thread_exit_sync);
}

static void _destroy_all() {
  _databuf_destroy();
  _mutex_destroy();
  _sem_destroy();
  _audioout_final();
}

void AudioPlayerFinal(void) {
  _worker_thread_exit();
  _destroy_all();
}

Result AudioPlayerSetFrontType(AudioPlayerType type, float ratio) {
  uni_pthread_mutex_lock(g_audio_player.mutex);
  g_audio_player.cur_front_type = type;
  g_audio_player.front_audio_ratio[type] = ratio;
  uni_pthread_mutex_unlock(g_audio_player.mutex);
  LOGT(AUDIO_PLAYER_TAG, "front type=%d, ratio=%f", type, ratio);
  return E_OK;
}

Result AudioPlayerStart(AudioPlayerInputCb input_handler,
                        AudioPlayerType type) {
  if (type <= AUDIO_NULL_PLAYER || AUDIO_PLAYER_CNT <= type) {
    LOGE(AUDIO_PLAYER_TAG, "type[%d] invalid", type);
    return E_FAILED;
  }
  LOGE(AUDIO_PLAYER_TAG, "type[%d]", type);
  uni_pthread_mutex_lock(g_audio_player.mutex);
  g_audio_player.input_handler[type] = input_handler;
  g_audio_player.audio_player_playing[type] = TRUE;
  uni_pthread_mutex_unlock(g_audio_player.mutex);
  uni_sem_post(g_audio_player.wait_start);
  return E_OK;
}

Result AudioPlayerStop(AudioPlayerType type) {
  uni_pthread_mutex_lock(g_audio_player.mutex);
  g_audio_player.audio_player_playing[type] = FALSE;
  uni_pthread_mutex_unlock(g_audio_player.mutex);
  return E_OK;
}

uni_bool AudioPlayerIsActive(void) {
  return 0;
}

Result AudioVolumeSet(uni_s32 vol) {
  if (vol > VOLUME_MAX || vol < VOLUME_MIN) {
    LOGE(AUDIO_PLAYER_TAG, "set volume %d rejected", vol);
    return E_REJECT;
  }
  if (0 > uni_hal_audio_set_volume(g_audio_player.audioout_handler, vol)) {
    LOGE(AUDIO_PLAYER_TAG, "set volume %d failed", vol);
    return E_FAILED;
  }
  g_audio_player.volume = vol;
  LOGT(AUDIO_PLAYER_TAG, "set volume to %d", g_audio_player.volume);
  return E_OK;
}

uni_s32 AudioVolumeGet(void) {
  return g_audio_player.volume;
}

Result AudioVolumeInc(uni_s32 base) {
  if (base + VOLUME_STEP > VOLUME_MAX) {
    LOGE(AUDIO_PLAYER_TAG, "set volume rejected");
    return E_REJECT;
  }
  if (0 > uni_hal_audio_set_volume(g_audio_player.audioout_handler,
      base + VOLUME_STEP)) {
    LOGE(AUDIO_PLAYER_TAG, "set volume failed");
    return E_FAILED;
  }
  g_audio_player.volume = base + VOLUME_STEP;
  LOGT(AUDIO_PLAYER_TAG, "set volume to %d", g_audio_player.volume);
  return E_OK;
}

Result AudioVolumeDec(uni_s32 base) {
  if (base < VOLUME_MIN + VOLUME_STEP) {
    LOGE(AUDIO_PLAYER_TAG, "set volume rejected");
    return E_REJECT;
  }
  if (0 > uni_hal_audio_set_volume(g_audio_player.audioout_handler,
      base - VOLUME_STEP)) {
    LOGE(AUDIO_PLAYER_TAG, "set volume failed");
    return E_FAILED;
  }
  g_audio_player.volume = base - VOLUME_STEP;
  LOGT(AUDIO_PLAYER_TAG, "set volume to %d", g_audio_player.volume);
  return E_OK;
}
