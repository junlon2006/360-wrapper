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
 * Description : uni_media_player.c
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.08.02
 *
 **********************************************************************/

#include "../inc/uni_media_player.h"
#include "uni_audio_player.h"
#include "uni_mp3_player.h"
#include "uni_log.h"
#include "uni_iot.h"
#include "uni_types.h"

#define MEDIAL_PLAYER_TAG   "medial_player"

int MediaPlayerInit() {
  AudioPlayerParam param;
  param.channels = 1;
  param.rate = 16000;
  param.bit = 16;
  if (E_OK != AudioPlayerInit(&param)) {
    LOGE(MEDIAL_PLAYER_TAG, "audio play init failed");
    return -1;
  }
  if (E_OK != Mp3Init(&param)) {
    LOGE(MEDIAL_PLAYER_TAG, "mp3 init failed");
    return -1;
  }
  return 0;
}

void MediaPlayerFinal() {
  AudioPlayerFinal();
  Mp3Final();
}

int MediaPlay(char *url) {
  Mp3Play(url);
  return 0;
}

int MediaPause(void) {
  Mp3Pause();
  return 0;
}

int MediaResume(void) {
  Mp3Resume();
  return 0;
}

int MediaStop() {
  Mp3Stop();
  return 0;
}

int MediaPlayIsPlaying() {
  return Mp3CheckIsPlaying();
}

int MediaPlayIsPause() {
  return Mp3CheckIsPause();
}
