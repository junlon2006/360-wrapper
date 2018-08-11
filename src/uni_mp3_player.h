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
 * Description : uni_mp3_player.h
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#ifndef SDK_PLAYER_MP3_INC_UNI_MP3_PLAYER_H_
#define SDK_PLAYER_MP3_INC_UNI_MP3_PLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uni_iot.h"
#include "uni_audio_player.h"

Result Mp3Play(char *filename);
Result Mp3Start(void);
Result Mp3Pause(void);
Result Mp3Resume(void);
Result Mp3Stop(void);
Result Mp3SeekToMsec(int msec);
Result Mp3Init(AudioParam *param);
Result Mp3Final(void);

uni_bool Mp3CheckIsPlaying(void);
uni_bool Mp3CheckIsPause(void);

#ifdef __cplusplus
}
#endif
#endif  //  SDK_PLAYER_MP3_INC_UNI_MP3_PLAYER_H_
