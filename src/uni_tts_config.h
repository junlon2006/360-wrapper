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
 * Description : uni_tts_config.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.07.18
 *
 **************************************************************************/

#ifndef SDK_PLAYER_TTS_SRC_UNI_TTS_CONFIG_H_
#define SDK_PLAYER_TTS_SRC_UNI_TTS_CONFIG_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "uni_iot.h"
#include "uni_types.h"

#define TTS_KEY_LEN           (64)
#define TTS_SEC_LEN           (64)
#define TTS_SPEAKER_LEN       (16)
#define REC_PATH_LEN          (32)

typedef struct {
  uni_u32           host; /* tts server ip */
  uni_u16           port;
  uni_inter_proto_t protocol;
  uni_s32           encode;
  char              appkey[TTS_KEY_LEN];
  char              secretkey[TTS_SEC_LEN];
  char              speaker[TTS_SPEAKER_LEN];
  uni_s32           spd; /* speed, scale: 0 - 100 */
  uni_s32           vol; /* volume, 0 - 100 */
  uni_s32           pit; /* pitch, 0 - 100 */
  uni_s32           emt; /* end silence time, unit: ms, 0-1000 */
  uni_s32           smt; /* start silence time, unit: ms, 0-1000 */
  uni_s32           save_file; /* save lasr record */
  char              save_path[REC_PATH_LEN]; /* save lasr record path */
} TtsParam;

Result TtsConfigLoad(const char *config_file, TtsParam *param);

#ifdef __cplusplus
}
#endif
#endif
