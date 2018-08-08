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
 * Description : uni_tts_config.c
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.07.18
 *
 **************************************************************************/

#include "uni_tts_config.h"

#include "uni_config.h"
#include "uni_log.h"

#define TTS_CONFIG_TAG  "tts_config"

Result TtsConfigLoad(const char *config_file, TtsParam *param) {
  uni_s64 number;
  ConfigReadItemNumber(config_file, "tts.host", &number);
  param->host = number;
  LOGD(TTS_CONFIG_TAG, "tts.host=%d", param->host);
  ConfigReadItemNumber(config_file, "tts.port", &number);
  param->port = number;
  LOGD(TTS_CONFIG_TAG, "tts.port=%d", param->port);
  ConfigReadItemNumber(config_file, "tts.protocol", &number);
  param->protocol = number;
  LOGD(TTS_CONFIG_TAG, "tts.protocol=%d", param->protocol);
  ConfigReadItemNumber(config_file, "tts.encode", &number);
  param->encode = number;
  LOGD(TTS_CONFIG_TAG, "tts.encode=%d", param->encode);
  ConfigReadItemNumber(config_file, "tts.spd", &number);
  param->spd = number;
  LOGD(TTS_CONFIG_TAG, "tts.spd=%d", param->spd);
  ConfigReadItemNumber(config_file, "tts.vol", &number);
  param->vol = number;
  LOGD(TTS_CONFIG_TAG, "tts.vol=%d", param->vol);
  ConfigReadItemNumber(config_file, "tts.pit", &number);
  param->pit = number;
  LOGD(TTS_CONFIG_TAG, "tts.pit=%d", param->pit);
  ConfigReadItemNumber(config_file, "tts.emt", &number);
  param->emt = number;
  LOGD(TTS_CONFIG_TAG, "tts.emt=%d", param->emt);
  ConfigReadItemNumber(config_file, "tts.smt", &number);
  param->smt = number;
  LOGD(TTS_CONFIG_TAG, "tts.smt=%d", param->smt);
  ConfigReadItemNumber(config_file, "tts.save_file", &number);
  param->save_file = number;
  LOGD(TTS_CONFIG_TAG, "tts.save_file=%d", param->save_file);
  ConfigReadItemString(config_file, "tts.appkey", param->appkey,
                       sizeof(param->appkey));
  LOGD(TTS_CONFIG_TAG, "tts.appkey=%s", param->appkey);
  ConfigReadItemString(config_file, "tts.secretkey", param->secretkey,
                       sizeof(param->secretkey));
  LOGD(TTS_CONFIG_TAG, "tts.secretkey=%s", param->secretkey);
  ConfigReadItemString(config_file, "tts.speaker", param->speaker,
                       sizeof(param->speaker));
  LOGD(TTS_CONFIG_TAG, "tts.speaker=%s", param->speaker);
  ConfigReadItemString(config_file, "tts.save_path", param->save_path,
                       sizeof(param->save_path));
  LOGD(TTS_CONFIG_TAG, "tts.save_path=%s", param->save_path);
  return E_OK;
}