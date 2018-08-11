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
 * Description : uni_lasr.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.07.31
 *
 **********************************************************************/

#include "../inc/uni_lasr.h"
#include "uni_log.h"
#include "uni_iot.h"
#include "uni_types.h"
#include "uni_audio_player.h"
#include "uni_mp3_player.h"
#include "uni_tts_player.h"
#include "uni_config.h"
#include <stdio.h>
#include <unistd.h>

#define MAIN_TAG        "main"
#define RESOURCE_PATH   "/run/uniapp"

typedef enum {
  MUSIC_PLAYING,
  MUSIC_PAUSE,
  MUSIC_IDLE
} MusicState;

static void _music_test(char *command) {
  static MusicState state = MUSIC_IDLE;
  if (0 == strcmp(command, "我要听歌")) {
    LOGW(MAIN_TAG, "play music");
    if (MUSIC_IDLE == state) {
      Mp3Play("http://m128.xiami.net/158/7158/2103755363/180336"
              "5859_1529979280707.mp3?auth_key=1534474800-0-0-d84"
              "3d147ee52deee4bbc9c53f0c28aa8");
      state = MUSIC_PLAYING;
    } else {
      LOGW(MAIN_TAG, "invalid state");
    }
    return;
  }
  if (0 == strcmp(command, "停止播放")) {
    LOGW(MAIN_TAG, "stop music");
    if (state != MUSIC_IDLE) {
      Mp3Stop();
      state = MUSIC_IDLE;
    } else {
      LOGW(MAIN_TAG, "invalid state");
    }
    return;
  }
  if (0 == strcmp(command, "继续播放")) {
    LOGW(MAIN_TAG, "resume music");
    if (state == MUSIC_PAUSE) {
      Mp3Resume();
      state = MUSIC_PLAYING;
    } else {
      LOGW(MAIN_TAG, "invalid state");
    }
    return;
  }
  if (0 == strcmp(command, "暂停播放")) {
    if (state == MUSIC_PLAYING) {
      Mp3Pause();
      state = MUSIC_PAUSE;
    } else {
      LOGW(MAIN_TAG, "invalid state");
    }
    LOGW(MAIN_TAG, "pause music");
    return;
  }
}

static void _get_audio_source(char *raw_pcm, int bytes_len,
                              LasrResult *result, int vad_end) {
  if (result) {
    LOGT(MAIN_TAG, "get asr result. keyword=%s, keyword_audio=%p, "
         "keyword_audio_len=%d, raw_audio_len=%d", result->keyword,
         result->audio_contain_keyword,
         result->audio_len, bytes_len);
    _music_test(result->keyword);
  }
  if (vad_end && NULL == result) {
    LOGT(MAIN_TAG, "SPEECH END...");
    LasrWakeupReset();
  }
}

int main() {
  LasrParam lasr_param;
  lasr_param.frame_size_msec = 40;
  if (0 != LasrInit(RESOURCE_PATH, &lasr_param, _get_audio_source)) {
    LOGE(MAIN_TAG, "lasr init failed");
    return -1;
  }
  LOGT(MAIN_TAG, "360 demo start successfully");
  while (1) {
    usleep(1000 * 1000);
  }
  LasrFinal();
  return 0;
}
