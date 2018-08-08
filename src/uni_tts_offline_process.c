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
 * Description : uni_tts_offline_process.c
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.07.18
 *
 **************************************************************************/

#include "uni_tts_offline_process.h"

#include "uni_log.h"
#include "yzsttshandle.h"

#define TTS_OFFLINE_TAG          "tts_offline"
#define TTS_OFFLINE_SAMPLE_LEN   (16 * 500)

#if UNI_OFFLINE_TTS_MODULE

typedef struct {
  void *offline_handle;
} TtsOffline;

static TtsOffline g_offline = {NULL};

Result TtsOfflineInit(TtsParam *param) {
  uni_s32 ret = 0;
  char buf[20] = {0};
  const char *backend_model = "";
  const char *frontend_model = "models/frontend_model_offline.lts";
  const char *marked_model = "";
  const char *user_dict_model = "models/user_data.lts";
#ifdef PLATFORM_RK3229
  char *pwd = "20180316161813";
  extern int yzstts_set_env(void *env);
  if (0 != yzstts_set_env((void *)pwd)) {
    LOGE(TTS_OFFLINE_TAG, "set offline tts env failed");
    return E_FAILED;
  }
  LOGD(TTS_OFFLINE_TAG, "set offline tts env success");
#endif
  if (0 == uni_strcmp(param->speaker, "boy")) {
    backend_model = "models/little_boy_backend_lpc2wav_22k_24d_pf.lts";
  } else if (0 == uni_strcmp(param->speaker, "tangtang")) {
    backend_model = "models/little_girl_backend_lpc2wav_22k_24d_pf.lts";
  } else if (0 == uni_strcmp(param->speaker, "xiaoming")) {
    backend_model = "models/male_backend_lpc2wav_22k_24d_pf.lts";
  } else if (0 == uni_strcmp(param->speaker, "xiaowen")) {
    backend_model = "models/backend_xiaowen_lpc2wav_22k_pf_mixed.lts";
  } else if (0 == uni_strcmp(param->speaker, "kiyo")) {
    backend_model = "models/backend_kiyo_lpc2wav_16k_pf_mixed.lts";
  } else {
    backend_model = "models/female_backend_lpc2wav_22k_24d_pf.lts";
  }
  LOGW(TTS_OFFLINE_TAG, "frontend_model=%s, backend_model=%s, "
       "marked_model=%s, user_dict_model=%s",
       frontend_model, backend_model, marked_model, user_dict_model);
  g_offline.offline_handle = yzstts_create_singleton(frontend_model,
                                                     backend_model,
                                                     marked_model,
                                                     user_dict_model);
  if (NULL == g_offline.offline_handle) {
    LOGE(TTS_OFFLINE_TAG, "offline tts create failed");
    return E_FAILED;
  }
  ret = yzstts_set_option(g_offline.offline_handle, 1,
                          uni_itoa(param->spd, buf, sizeof(buf)));
  ret |= yzstts_set_option(g_offline.offline_handle, 9, "16000");
  ret |= yzstts_set_option(g_offline.offline_handle, 6, "20");
  ret |= yzstts_set_option(g_offline.offline_handle, 7, "20");
  if (ret != 0) {
    LOGE(TTS_OFFLINE_TAG, "offline tts set option failed");
    return E_FAILED;
  }
  return E_OK;
}

Result TtsOfflineFinal() {
  if (NULL != g_offline.offline_handle) {
    yzstts_release_singleton(g_offline.offline_handle);
    g_offline.offline_handle = NULL;
  }
  return E_OK;
}

Result TtsOfflineProcess(char *content, DataBufHandle databuf,
                         uni_bool *force_stop, TtsParam *param) {
  uni_s16 *wavbuf = NULL;
  char *data = NULL;
  uni_s32 audio_len = 0;
  uni_s32 samplenum = 0;
  uni_s32 free_size = 0;
  uni_s32 write_size = 0;
  uni_s32 first = 0;
  if (0 != yzstts_set_text(g_offline.offline_handle, content)) {
    LOGE(TTS_OFFLINE_TAG, "offline tts set text failed");
    return E_FAILED;
  }
  if (NULL == (wavbuf = uni_malloc(TTS_OFFLINE_SAMPLE_LEN * sizeof(uni_s16)))) {
    LOGE(TTS_OFFLINE_TAG, "alloc memory failed");
    return E_FAILED;
  }
  while (!*force_stop) {
    samplenum = yzstts_generate_wave(g_offline.offline_handle, wavbuf,
                                     TTS_OFFLINE_SAMPLE_LEN);
    if (samplenum <= 0) {
      LOGE(TTS_OFFLINE_TAG, "offline tts generate wave finish[%d]", samplenum);
      break;
    }
    if (++first == 1) {
      LOGW(TTS_OFFLINE_TAG, "first offline wav generate[samplenum=%d]",
           samplenum);
    }
    audio_len = samplenum * sizeof(uni_s16);
    data = (char *)wavbuf;
    while (0 < audio_len && !*force_stop) {
      free_size = DataBufferGetFreeSize(databuf);
      write_size = uni_min(free_size, audio_len);
      if (0 < write_size) {
        DataBufferWrite(databuf, data, write_size);
        audio_len -= write_size;
        data += write_size;
        continue;
      }
      uni_msleep(5);
    }
  }
  free(wavbuf);
  return E_OK;
}

#else

Result TtsOfflineInit(TtsParam *param) {return E_OK;}
Result TtsOfflineFinal() {return E_OK;}
Result TtsOfflineProcess(char *content, DataBufHandle databuf,
                         uni_bool *force_stop, TtsParam *param) {return E_OK;}
#endif
