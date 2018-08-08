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
 * Description : uni_tts_online_process.c
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.07.18
 *
 **************************************************************************/

#include "uni_tts_online_process.h"

#include "uni_log.h"
#include "tts_sdk.h"
#include "uni_tcp_ser.h"

#define TTS_ONLINE_TAG   "tts_online"

#if UNI_ONLINE_TTS_MODULE

typedef struct {
  char speaker[TTS_SPEAKER_LEN];
} TtsOnline;

static TtsOnline g_tts_online = {0};

static uni_s32 _inet_ntoa(struct uni_in_addr ipints, char *addr, uni_s32 len) {
  if (NULL == addr || 0 == (uni_u32)ipints.s_addr || len < 16) {
    return E_FAILED;
  }
  uni_memset(addr, 0, len);
  char *tmp = inet_ntoa(ipints);
  if (NULL != tmp) {
    uni_memcpy(addr, tmp, len);
    return E_OK;
  }
  return E_FAILED;
}

Result TtsOnlineProcess(char *content, DataBufHandle databuf,
                        uni_bool *force_stop, TtsParam *param) {
  uni_s32 ret = 0;
  uni_u32 audio_len;
  uni_s32 synth_status = RECEIVING_AUDIO_DATA;
  struct uni_in_addr host;
  char ip_str[16] = {0,};
  char buf[128] = {0,};
  uni_s32 s_time = 0;
  TTSHANDLE handle = 0;
  uni_s32 err = 0;
  s_time = uni_get_now_msec();
  host.s_addr = (param->host);
  _inet_ntoa(host, ip_str, sizeof(ip_str));
  ret = usc_tts_create_service_ext(&handle, ip_str, uni_itoa(param->port,
                                   buf, sizeof(buf)));
  if (ret != USC_SUCCESS) {
    LOGE(TTS_ONLINE_TAG, "login failed. error code=%d", ret);
    return E_FAILED;
  }
  ret |= usc_tts_set_option(handle, "appkey", param->appkey);
  ret |= usc_tts_set_option(handle, "secret", param->secretkey);
  ret |= usc_tts_set_option(handle, "vcn",
                            0 == uni_strlen(g_tts_online.speaker) ?
                            param->speaker : g_tts_online.speaker);
  ret |= usc_tts_set_option(handle, "spd", uni_itoa(param->spd, buf, sizeof(buf)));
  ret |= usc_tts_set_option(handle, "vol", uni_itoa(param->vol, buf, sizeof(buf)));
  ret |= usc_tts_set_option(handle, "pit", uni_itoa(param->pit, buf, sizeof(buf)));
  ret |= usc_tts_set_option(handle, "emt", uni_itoa(param->emt, buf, sizeof(buf)));
  ret |= usc_tts_set_option(handle, "smt", uni_itoa(param->smt, buf, sizeof(buf)));
  ret |= usc_tts_set_option(handle, "audioFormat", "audio/x-wav");
  ret |= usc_tts_set_option(handle, "audioCodec", "opus");
  ret |= usc_tts_start_synthesis(handle);
  if (ret != USC_SUCCESS) {
    LOGE(TTS_ONLINE_TAG, "begin session failed. error code=%d", ret);
    usc_tts_release_service(handle);
    return E_FAILED;
  }
  ret = usc_tts_text_put(handle, content, uni_strlen(content) + 1);
  if (ret != USC_SUCCESS) {
    LOGE(TTS_ONLINE_TAG, "put text failed. error code=%d", ret);
    usc_tts_stop_synthesis(handle);
    usc_tts_release_service(handle);
    return E_FAILED;
  }
  LOGD(TTS_ONLINE_TAG, "tts http session_id=%s", usc_tts_get_option(handle, 21, &ret));
  LOGD(TTS_ONLINE_TAG, "tts http start cost %dms.",
       uni_get_now_msec() - s_time);
  while (!*force_stop) {
    char *data;
    uni_s32 free_size;
    uni_s32 write_size;
    audio_len = 0;
    data = usc_tts_get_result(handle, &audio_len, &synth_status, &ret);
    if (synth_status == AUDIO_DATA_RECV_DONE || ret != 0) {
      if (ret != 0) {
        LOGE(TTS_ONLINE_TAG, "tts get result failed. rc=%d", ret);
      }
      break;
    }
    while (0 < audio_len && NULL != data && !*force_stop) {
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
  if ((err = usc_tts_stop_synthesis(handle)) != USC_SUCCESS) {
    LOGE(TTS_ONLINE_TAG, "stop failed. error code=%d", err);
  }
  usc_tts_release_service(handle);
  return E_OK;
}

Result TtsOnlineSetSpeaker(const char *speaker) {
  uni_strncpy(g_tts_online.speaker, speaker, sizeof(g_tts_online.speaker));
}

#else

Result TtsOnlineProcess(char *content, DataBufHandle databuf,
                        uni_bool *force_stop, TtsParam *param) {return E_OK;}
Result TtsOnlineSetSpeaker(const char *speaker) {return E_OK;}

#endif
