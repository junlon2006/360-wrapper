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
#include <stdio.h>
#include <unistd.h>

#define MAIN_TAG        "main"
#define RESOURCE_PATH   "/oem/uniapp"

static void _get_audio_source(char *raw_pcm, int bytes_len,
                              LasrResult *result, int vad_end) {
  if (result) {
    LOGT(MAIN_TAG, "get asr result. keyword=%s, keyword_audio=%p, "
         "keyword_audio_len=%d, raw_audio_len=%d", result->keyword, result->audio_contain_keyword,
         result->audio_len, bytes_len);
  }
  if (vad_end) {
    LOGT(MAIN_TAG, "SPEECH END...");
  }
}

int main() {
  LasrParam lasr_param;
  lasr_param.frame_size_msec = 50;
  if (0 != LasrInit(RESOURCE_PATH, &lasr_param, (CbAudioSource)_get_audio_source)) {
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
