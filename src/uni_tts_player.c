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
 * Description : uni_tts_player.c
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/

#include "uni_tts_player.h"

#include "uni_log.h"
#include "list_head.h"
#include "uni_databuf.h"
#include "uni_tts_config.h"
#include "uni_event.h"
#include "uni_audio_player.h"
#include "uni_tts_online_process.h"
#include "uni_tts_offline_process.h"

#define TTS_PLAYER_TAG      "tts_player"
#define TTS_DATABUF_SIZE    (16 * 1024)

typedef struct {
  list_head link;
  TtsType   type;
  char      *content;
} TtsPlayContent;

typedef enum {
  TTS_START = 0,
  TTS_END,
} TtsState;

typedef struct {
  list_head     content_list;
  uni_mutex_t   lock;
  uni_sem_t     sem_data;
  uni_sem_t     sem_thread_exit_sync;
  uni_bool      is_running;
  uni_bool      synthesis_force_stoped;
  uni_pthread_t pid;
  DataBufHandle databuf;
  TtsState      state;
  TtsParam      *param;
} TtsPlayer;

static TtsPlayer g_tts_player;

static TtsPlayContent *_get_play_content() {
  list_head *p;
  TtsPlayContent *content = NULL;
  uni_sem_wait(g_tts_player.sem_data);
  if (NULL == (p = list_get_head(&g_tts_player.content_list))) {
    return NULL;
  }
  content = list_entry(p, TtsPlayContent, link);
  return content;
}

static Result _synthesis(TtsPlayContent *content) {
  Result rc;
  if (TTS_ONLINE == content->type) {
    rc = TtsOnlineProcess(content->content, g_tts_player.databuf,
                          &g_tts_player.synthesis_force_stoped,
                          g_tts_player.param);
  } else {
    rc = TtsOfflineProcess(content->content, g_tts_player.databuf,
                           &g_tts_player.synthesis_force_stoped,
                           g_tts_player.param);
  }
  return rc;
}

static void _free_play_content(TtsPlayContent *content) {
  list_del(&content->link);
  uni_free(content->content);
  uni_free(content);
}

static TtsPlayContent *_alloc_play_content(char *play_content, TtsType type) {
  TtsPlayContent *content = NULL;
  if (NULL == (content = uni_malloc(sizeof(TtsPlayContent)))) {
    LOGE(TTS_PLAYER_TAG, "alloc memory failed");
    return NULL;
  }
  content->type = type;
  if (NULL == (content->content = uni_malloc(uni_strlen(play_content) + 1))) {
    uni_free(content);
    LOGE(TTS_PLAYER_TAG, "alloc memory failed");
    return NULL;
  }
  uni_strcpy(content->content, play_content);
  content->type = type;
  return content;
}

void _send_tts_start_event_2_route() {
}

static void _send_tts_end_event_2_route() {
  SendEvent(AUDIO_PLAY_TTS_END);
}

static void _set_tts_state(TtsState state) {
  g_tts_player.state = state;
}

static void _clear_audio_databuf() {
  DataBufferClear(g_tts_player.databuf);
}

static void _clear_all_play_content() {
  TtsPlayContent *p, *t;
  list_for_each_entry_safe(p, t, &g_tts_player.content_list,
                           TtsPlayContent, link) {
    list_del(&p->link);
    uni_free(p->content);
    uni_free(p);
  }
}

static uni_bool _is_synthesis_force_stoped() {
  uni_bool force_stoped = FALSE;
  uni_pthread_mutex_lock(g_tts_player.lock);
  if (g_tts_player.synthesis_force_stoped) {
    force_stoped = TRUE;
    _clear_all_play_content();
  }
  uni_pthread_mutex_unlock(g_tts_player.lock);
  return force_stoped;
}

static void _tts_synthesis_tsk(void *args) {
  TtsPlayContent *content = NULL;
  while (g_tts_player.is_running) {
    if (NULL == (content = _get_play_content())) {
      continue;
    }
    if (_is_synthesis_force_stoped()) {
      LOGT(TTS_PLAYER_TAG, "tts synthesis force stoped");
      continue;
    }
    _clear_audio_databuf();
    _set_tts_state(TTS_START);
    //_send_tts_start_event_2_route();
    _synthesis(content);
    _set_tts_state(TTS_END);
    _free_play_content(content);
  }
  uni_sem_post(g_tts_player.sem_thread_exit_sync);
}

static void _woker_thread_create() {
  struct thread_param param;
  uni_memset(&param, 0, sizeof(param));
  param.stack_size = STACK_SMALL_SIZE;
  param.th_priority = OS_PRIORITY_HIGH;
  uni_strncpy(param.task_name, "ttsplayer", sizeof(param.task_name));
  g_tts_player.is_running = TRUE;
  uni_pthread_create(&g_tts_player.pid, &param, _tts_synthesis_tsk, NULL);
  uni_pthread_detach(g_tts_player.pid);
}

static void _tts_play_content_list_init() {
  list_init(&g_tts_player.content_list);
}

static void _mutex_init() {
  uni_pthread_mutex_init(&g_tts_player.lock);
}

static void _sem_init() {
  uni_sem_init(&g_tts_player.sem_data, 0);
  uni_sem_init(&g_tts_player.sem_thread_exit_sync, 0);
}

static void _mutex_destroy() {
  uni_pthread_mutex_destroy(g_tts_player.lock);
}

static void _sem_destroy() {
  uni_sem_destroy(g_tts_player.sem_data);
  uni_sem_destroy(g_tts_player.sem_thread_exit_sync);
}

static void _databuf_create() {
  g_tts_player.databuf = DataBufferCreate(TTS_DATABUF_SIZE);
}

static void _databuf_destroy() {
  DataBufferDestroy(g_tts_player.databuf);
}

static void _tts_param_destroy() {
  uni_free(g_tts_player.param);
}

static void _tts_param_init(const char *config_file) {
  if (NULL == (g_tts_player.param = uni_malloc(sizeof(TtsParam)))) {
    LOGE(TTS_PLAYER_TAG, "alloc memory failed");
    return;
  }
  TtsConfigLoad(config_file, g_tts_player.param);
}

Result TtsCreate(const char *config_file) {
  LOGT(TTS_PLAYER_TAG, "config_file=%s", config_file);
  uni_memset(&g_tts_player, 0x0, sizeof(TtsPlayer));
  _tts_play_content_list_init();
  _mutex_init();
  _sem_init();
  _databuf_create();
  _woker_thread_create();
  _tts_param_init(config_file);
  TtsOfflineInit(g_tts_player.param);
  return E_OK;
}

static void _destroy_all() {
  _mutex_destroy();
  _sem_destroy();
  _databuf_destroy();
  _tts_param_destroy();
  TtsOfflineFinal();
}

Result TtsDestroy(void) {
  g_tts_player.is_running = FALSE;
  uni_sem_wait(g_tts_player.sem_thread_exit_sync);
  _destroy_all();
  return E_OK;
}

static uni_s32 _audio_data_retrieve(DataBufHandle handle) {
  uni_s32 free_size;
  uni_s32 cur_read_size;
  uni_s32 remain_len;
  char *buf = NULL;
  if (TTS_END == g_tts_player.state &&
      0 == DataBufferGetDataSize(g_tts_player.databuf)) {
    LOGT(TTS_PLAYER_TAG, "tts retrieve end");
    _send_tts_end_event_2_route();
    return -1;
  }
  if ((free_size = DataBufferGetFreeSize(handle)) <= 0) {
    LOGW(TTS_PLAYER_TAG, "free_size=%d", free_size);
    uni_msleep(5);
    return 0;
  }
  if (0 == (remain_len = DataBufferGetDataSize(g_tts_player.databuf))) {
    uni_msleep(5);
    return 0;
  }
  cur_read_size = uni_min(free_size, remain_len);
  buf = uni_malloc(cur_read_size);
  DataBufferRead(buf, cur_read_size, g_tts_player.databuf);
  DataBufferWrite(handle, buf, cur_read_size);
  uni_free(buf);
  return cur_read_size;
}

Result TtsPlayString(char *play_content, TtsType type) {
  TtsPlayContent *content = NULL;
  if (NULL == play_content ||
      0 == uni_strlen(play_content) ||
      (type != TTS_ONLINE && type != TTS_OFFLINE)) {
    LOGE(TTS_PLAYER_TAG, "param invalid, play_content=%p, type=%d",
         play_content, type);
  }
  if (NULL == (content = _alloc_play_content(play_content, type))) {
    LOGE(TTS_PLAYER_TAG, "alloc play content failed");
    return E_FAILED;
  }
  LOGT(TTS_PLAYER_TAG, "ttsplayer:%s", play_content);
  uni_pthread_mutex_lock(g_tts_player.lock);
  list_add_tail(&content->link, &g_tts_player.content_list);
  g_tts_player.synthesis_force_stoped = FALSE;
  uni_pthread_mutex_unlock(g_tts_player.lock);
  uni_sem_post(g_tts_player.sem_data);
  AudioPlayerStart(_audio_data_retrieve, AUDIO_TTS_PLAYER);
  return E_OK;
}

Result TtsStop() {
  uni_pthread_mutex_lock(g_tts_player.lock);
  g_tts_player.synthesis_force_stoped = TRUE;
  uni_pthread_mutex_unlock(g_tts_player.lock);
  AudioPlayerStop(AUDIO_TTS_PLAYER);
  return E_OK;
}

Result TtsSetSpeaker(const char *speaker) {
  return TtsOnlineSetSpeaker(speaker);
}
