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
 * Description : main.c
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
#include "uni_mp3_parse_pcm.h"
#include "uni_pcm_player.h"
#include "uni_event.h"

#define MAIN_TAG        "main"
#define RESOURCE_PATH   "."
#define TEST_2_MP3_MIX  (0)

typedef enum {
  MUSIC_PLAYING,
  MUSIC_PAUSE,
  MUSIC_IDLE
} MusicState;

static MusicState state = MUSIC_IDLE;

static void _music_test(char *command) {
  if (0 == strcmp(command, "小宝小宝")) {
    LOGT(MAIN_TAG, "tts speaker: boy");
    TtsSetSpeaker("boy");
    TtsPlay("你好，我在", TTS_ONLINE);
    AudioPlayerSetFrontType(AUDIO_TTS_PLAYER, 0.8);
    return;
  }
  if (0 == strcmp(command, "小贝小贝")) {
    LOGT(MAIN_TAG, "tts speaker: lzl");
    TtsSetSpeaker("kiyo");
    TtsPlay("你好，我在", TTS_ONLINE);
    AudioPlayerSetFrontType(AUDIO_TTS_PLAYER, 0.8);
    return;
  }
  LasrWakeupReset();
  if (0 == strcmp(command, "我要听歌")) {
    LOGW(MAIN_TAG, "play music");
    if (MUSIC_IDLE == state) {
      Mp3Play("http://m128.xiami.net/198/1198/6067/74192_39943_l.mp3?"
              "auth_key=1535079600-0-0-0f227c0a896047f08d42f5a9352b0f71");
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
  if (0 == strcmp(command, "北京天气")) {
    TtsPlay("北京今天天气晴，33到37度，空气质量优，外出请注意安全", TTS_ONLINE);
    AudioPlayerSetFrontType(AUDIO_TTS_PLAYER, 0.8);
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
  }
}

#if TEST_2_MP3_MIX
static void _mp3_parse_pcm_callback(char *data, int len, Mp3ParseErrorCode rc) {
  static char *buf = NULL;
  static int read_len = 0;
  if (NULL == buf) {
    buf = malloc(1024 * 1024 * 10);
  }
  if (rc == MP3_PARSE_OK) {
    memcpy(buf + read_len, data, len);
    read_len += len;
  }
  if (rc == MP3_PARSE_DONE) {
    LOGE(MAIN_TAG, "start pcm player");
    PcmPlay(buf, read_len);
  }
}
#endif

static void _mp3_mix_test() {
#if TEST_2_MP3_MIX
  Mp3ParsePcm("http://m128.xiami.net/770/2110321770/2103794783/1803654324_"
              "1531289479477.mp3?auth_key=1534474800-0-0-b918c74460761ba8d"
              "e72dda444dbbdc8", _mp3_parse_pcm_callback);
#endif
  usleep(30 * 1000 * 1000);
  PcmStop();
}

static void _event_callback(EventType type) {
  LOGW(MAIN_TAG, "recv event %d[%s]", type, EventType2String(type));
  if (type == AUDIO_PLAY_MEDIA_END) {
    state = MUSIC_IDLE;
  }
}

int main() {
  LasrParam lasr_param;
  lasr_param.frame_size_msec = 40;
  EventTypeCallbackRegister((CbEventType)_event_callback);
  if (0 != LasrInit(RESOURCE_PATH, &lasr_param, _get_audio_source)) {
    LOGE(MAIN_TAG, "lasr init failed");
    return -1;
  }
  LOGT(MAIN_TAG, "360 demo start successfully");
  _mp3_mix_test();
  while (TRUE) {
    usleep(1000 * 1000 * 30);
  }
  LasrFinal();
  return 0;
}
