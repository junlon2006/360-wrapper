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
 * Description : uni_tts_player.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#ifndef SDK_PLAYER_TTS_INC_UNI_TTS_PLAYER_H_
#define SDK_PLAYER_TTS_INC_UNI_TTS_PLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uni_iot.h"
//#include "uni_event_list.h"

typedef enum {
  TTS_ONLINE,
  TTS_OFFLINE,
} TtsType;

#define EventSet_Tts ( \
  TTS_START_EVENT \
  TTS_END_EVENT \
)

Result TtsCreate(const char *config_file);
Result TtsDestroy(void);
Result TtsPlayString(char *play_str, TtsType tts_type);
Result TtsStop(void);
Result TtsSetSpeaker(const char *speaker);

#ifdef __cplusplus
}
#endif
#endif
