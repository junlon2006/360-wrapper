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

#define AUDIO_PLAYER_TAG   "aduio_player"
#define PCM_FRAME_SIZE     (640)

#define CAPTURE_PCM        (0)
#if CAPTURE_PCM
static int f_audio_out;
#endif

typedef enum {
  STATE_IDLE,
  STATE_ACTIVE,
  STATE_STOPPING
} AudioPlayerState;

static struct {
  DataBufHandle      databuf_handle;
  AudioPlayerState   state;
  AudioPlayerInputCb input_handler;
  void               *audioout_handler;
  uni_pthread_t      pid;
  uni_bool           running;
  uni_sem_t          wait_start;
  uni_sem_t          wait_stop;
  uni_sem_t          sem_thread_exit_sync;
  uni_mutex_t        mutex;
  uni_s32            volume;
} g_audio_player;

static void _set_audio_player_state(AudioPlayerState state) {
  LOGT(AUDIO_PLAYER_TAG, "state changes from %d to %d",
       g_audio_player.state, state);
  g_audio_player.state = state;
}

static uni_bool _check_audio_player_state(AudioPlayerState state) {
  return (g_audio_player.state == state);
}

static uni_s32 _audio_write(uni_u32 frames) {
  uni_u32 total_frames = 0;
  char write_buf[PCM_FRAME_SIZE];
  uni_u32 data_len;
  while (total_frames < frames) {
    data_len = DataBufferGetDataSize(g_audio_player.databuf_handle);
    if (data_len < PCM_FRAME_SIZE) {
      uni_s32 ret = g_audio_player.input_handler(g_audio_player.databuf_handle);
      if (ret < 0) {
        LOGW(AUDIO_PLAYER_TAG, "audio input finished or failed");
        memset(write_buf, 0, PCM_FRAME_SIZE);
        DataBufferRead(write_buf, data_len, g_audio_player.databuf_handle);
        uni_hal_audio_write(g_audio_player.audioout_handler, write_buf,
                            PCM_FRAME_SIZE);
        memset(write_buf, 0, PCM_FRAME_SIZE);
        uni_hal_audio_write(g_audio_player.audioout_handler, write_buf,
                            PCM_FRAME_SIZE);
        uni_hal_audio_write(g_audio_player.audioout_handler, write_buf,
                            PCM_FRAME_SIZE);
        uni_hal_audio_write(g_audio_player.audioout_handler, write_buf,
                            PCM_FRAME_SIZE);
#if CAPTURE_PCM
        uni_hal_fwrite(f_audio_out, write_buf, data_len);
#endif
        return -1;
      } else if (ret < PCM_FRAME_SIZE) {
        /* wait until next write */
        return 0;
      }
    }
    DataBufferPeek(write_buf, PCM_FRAME_SIZE, g_audio_player.databuf_handle);
#if CAPTURE_PCM
    uni_hal_fwrite(f_audio_out, write_buf, PCM_FRAME_SIZE);
#endif
    if (uni_hal_audio_write(g_audio_player.audioout_handler, write_buf,
                            PCM_FRAME_SIZE) < 0) {
      break;
    }
    DataBufferRead(write_buf, PCM_FRAME_SIZE, g_audio_player.databuf_handle);
    total_frames++;
  }
  return total_frames;
}

static void _player_start_internal(void) {
  uni_hal_audio_stop(g_audio_player.audioout_handler);
  uni_hal_audio_start(g_audio_player.audioout_handler);
}

static void _player_stop_internal(void) {
  DataBufferClear(g_audio_player.databuf_handle);
}

static void _audio_player_process(void *args) {
  g_audio_player.running = TRUE;
  while (g_audio_player.running) {
    uni_sem_wait(g_audio_player.wait_start);
    _player_start_internal();
    LOGT(AUDIO_PLAYER_TAG, "playing started!!!");
    while (TRUE) {
      if (_check_audio_player_state(STATE_STOPPING)) {
        _set_audio_player_state(STATE_IDLE);
        uni_sem_post(g_audio_player.wait_stop);
        LOGT(AUDIO_PLAYER_TAG, "playing is force stopped");
        break;
      }
      if (_check_audio_player_state(STATE_IDLE)) {
        break;
      }
      if (-1 == _audio_write(5)) {
        uni_pthread_mutex_lock(g_audio_player.mutex);
        if (_check_audio_player_state(STATE_ACTIVE)) {
          _set_audio_player_state(STATE_IDLE);
        }
        uni_pthread_mutex_unlock(g_audio_player.mutex);
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
  g_audio_player.databuf_handle = DataBufferCreate(4096);
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
  DataBufferDestroy(g_audio_player.databuf_handle);
}

static void _mutex_destroy() {
  uni_pthread_mutex_destroy(g_audio_player.mutex);
}

static void _sem_destroy() {
  uni_sem_destroy(g_audio_player.wait_start);
  uni_sem_destroy(g_audio_player.wait_stop);
  uni_sem_destroy(g_audio_player.sem_thread_exit_sync);
}

Result AudioPlayerInit(AudioPlayerParam *param) {
  uni_memset(&g_audio_player, 0, sizeof(g_audio_player));
  _databuf_create();
  _mutex_init();
  _sem_init();
  _set_audio_player_state(STATE_IDLE);
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
  _set_audio_player_state(STATE_IDLE);
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

Result AudioPlayerStart(AudioPlayerInputCb input_handler) {
  if (_check_audio_player_state(STATE_ACTIVE)) {
    LOGW(AUDIO_PLAYER_TAG, "wrong state");
    return E_FAILED;
  }
#if CAPTURE_PCM
  f_audio_out = uni_hal_fopen("audio_out.pcm", O_CREAT | O_RDWR);
#endif
  g_audio_player.input_handler = input_handler;
  _set_audio_player_state(STATE_ACTIVE);
  /* clear history audio_end events which may affect the current playing */
  uni_sem_post(g_audio_player.wait_start);
  return E_OK;
}

Result AudioPlayerStop(void) {
  uni_pthread_mutex_lock(g_audio_player.mutex);
  if (!_check_audio_player_state(STATE_ACTIVE)) {
    LOGW(AUDIO_PLAYER_TAG, "wrong state");
    uni_pthread_mutex_unlock(g_audio_player.mutex);
    return E_FAILED;
  }
#if CAPTURE_PCM
  uni_hal_fclose(f_audio_out);
#endif
  _set_audio_player_state(STATE_STOPPING);
  uni_pthread_mutex_unlock(g_audio_player.mutex);
  uni_sem_wait(g_audio_player.wait_stop);
  return E_OK;
}

uni_bool AudioPlayerIsActive(void) {
  return (_check_audio_player_state(STATE_ACTIVE));
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
