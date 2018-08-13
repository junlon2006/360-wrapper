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
 * Description : uni_pcm_player.c
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#include "uni_audio_player.h"
#include "uni_log.h"
#include "uni_databuf.h"

#define MAX_PCM_FILE_LIST_LEN (10)
#define PCM_PLAYER_TAG        "pcm_player"

typedef struct {
  char            *buf;
  uni_s32         len;
  uni_s32         cur_pos;
  pthread_mutex_t mutex;
} PcmPlayer;

static PcmPlayer g_pcm_player = {0};

static uni_s32 _audio_player_callback(DataBufHandle handle) {
  uni_s32 free_size;
  uni_s32 read_size;
  pthread_mutex_lock(&g_pcm_player.mutex);
  if (g_pcm_player.cur_pos == g_pcm_player.len) {
    LOGT(PCM_PLAYER_TAG, "pcm play finish");
    free(g_pcm_player.buf);
    pthread_mutex_unlock(&g_pcm_player.mutex);
    return -1;
  }
  if ((free_size = DataBufferGetFreeSize(handle)) <= 0) {
    pthread_mutex_unlock(&g_pcm_player.mutex);
    return 0;
  }
  read_size = uni_min(free_size, g_pcm_player.len - g_pcm_player.cur_pos);
  DataBufferWrite(handle, g_pcm_player.buf + g_pcm_player.cur_pos, read_size);
  g_pcm_player.cur_pos += read_size;
  pthread_mutex_unlock(&g_pcm_player.mutex);
  return read_size;
}

Result PcmPlay(char *buf, int len) {
  pthread_mutex_lock(&g_pcm_player.mutex);
  g_pcm_player.buf = (char *)malloc(len);
  g_pcm_player.len = len;
  g_pcm_player.cur_pos = 0;
  memcpy(g_pcm_player.buf, buf, len);
  AudioPlayerStart(_audio_player_callback, AUDIO_PCM_PLAYER);
  pthread_mutex_unlock(&g_pcm_player.mutex);
  return E_OK;
}

Result PcmStop(void) {
  pthread_mutex_lock(&g_pcm_player.mutex);
  g_pcm_player.len = 0;
  g_pcm_player.cur_pos = 0;
  free(g_pcm_player.buf);
  AudioPlayerStop(AUDIO_PCM_PLAYER);
  pthread_mutex_unlock(&g_pcm_player.mutex);
  LOGT(PCM_PLAYER_TAG, "pcm stop");
  return E_OK;
}
