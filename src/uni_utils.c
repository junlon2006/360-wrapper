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
 * Description : uni_utils.c
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.08.11
 *
 **********************************************************************/

#include "uni_iot.h"
#include "uni_types.h"
#include "uni_log.h"
#include "uni_config.h"
#include "uni_tts_player.h"
#include "uni_audio_player.h"
#include "uni_mp3_player.h"

#define UTILS_TAG   "utils"

static Result _audio_out_init() {
  AudioPlayerParam param;
  param.channels = 1;
  param.rate = 16000;
  param.bit = 16;
  if (E_OK != AudioPlayerInit(&param)) {
    LOGE(UTILS_TAG, "audio play init failed");
    return E_FAILED;
  }
  if (E_OK != Mp3Init(&param)) {
    LOGE(UTILS_TAG, "mp3 init failed");
    return E_FAILED;
  }
  return E_OK;
}

int UtilsInit(const char *resource_file_path) {
  char tts_config_file_name[256];
  LogInitialize(NULL);
  ConfigInitialize();
  _audio_out_init();
  snprintf(tts_config_file_name, sizeof(tts_config_file_name),
           "%s/config/config_file", resource_file_path);
  TtsCreate(tts_config_file_name);
  TtsSetSpeaker("lzl");
  TtsPlayString("欢迎使用云知声智能语音产品", TTS_ONLINE);
  LOGT(UTILS_TAG, "utils init successfully");
  return 0;
}

int UtilsFinal() {
  TtsDestroy();
  ConfigFinalize();
  LogFinalize();
  return 0;
}
