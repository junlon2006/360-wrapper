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
 * Description : uni_lasr.c
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.07.31
 *
 **********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include "../inc/uni_lasr.h"
#include "uni_log.h"
#include "LinuxAsrFix.h"
#include "ofa_consts.h"
#include "uni_iot.h"
#include "uni_types.h"
#include "uni_dsp.h"
#include "uni_databuf.h"

#define LASR_TAG                    "lasr"
#define LASR_WAKEUP_THRESHOLD       (-20.0)
#define READ_DATA_DEFAULT_LEN       (65532)
#define TIMESTAMP_INVALID           (0)
#define CACHE_DATA_LEN              (10000 * 2 * 16)
#define SLICE_DATA_LEN              (512 * 10)
#define RECORD_DEBUG_OPEN           (0)

typedef struct {
  int start_frame_id;
  int latest_frame_id;
} Range;

typedef struct {
  int        vad_end;
  int        keyword_detected;
  LasrResult lasr_result;
  int        keyword_start_frame_id;
  int        keyword_end_frame_id;
} SliceParam;

typedef struct {
  CbAudioSource   cb_audio_source;
  int             need_calc_doa;
  unsigned int    asr_start_frame_id;
  int             is_wakeup;
  DataBufHandle   cache_databuf;
  DataBufHandle   slice_databuf;
  Range           cache_frame_range;
  LasrParam       lasr_param;
  SliceParam      slice_param;
  pthread_mutex_t mutex;
} Lasr;

static Lasr g_lasr;

static int _engine_init(const char *resource_file_path) {
  char model_path[256];
  char model_files[1024];
  const char *domains = "wakeup";
  int rc = -1;
  LOGD(LASR_TAG, "resource_file_path=%s", resource_file_path);
  snprintf(model_path, sizeof(model_path), "%s/models", resource_file_path);
  snprintf(model_files, sizeof(model_files), "%s/recognize.dat,%s/wakeup.dat",
           model_path, model_path);
  if (0 != (rc = init(model_path, model_files, 0))) {
    LOGD(LASR_TAG, "engine init failed[%d]", rc);
    return -1;
  }
  setOptionInt(ASR_SILENCE_HANGOUT_THRESH, 10);
  setOptionInt(ASR_ENGINE_SET_RESET_FRONTEND_ID, 1);
  setOptionInt(ASR_ENGINE_SET_FAST_RETURN, 1);
  setOptionInt(ASR_CONF_OUT_INELIGIBLE, 1);
  setOptionInt(ASR_ENGINE_SET_TYPE_ID, 2);
  setOptionInt(ASR_ENGINE_ENABLE_VAD_ID, 1);
  setOptionInt(ASR_LOG_ID, 1);
  if (0 != (rc = start(domains, 53))) {
    LOGE(LASR_TAG, "engine start failed[%d]", rc);
    return -1;
  }
  LOGD(LASR_TAG, "engine init successfully");
  return 0;
}

static void _set_calc_doa_status(int need_calc_doa) {
  g_lasr.need_calc_doa = need_calc_doa;
  LOGT(LASR_TAG, "calc doa status[%d]", need_calc_doa);
}

static void _recognize_result_parse(char *result, char *keyword, float *score) {
  /* TODO need parse result & score threshold */
  *score = -5.0;
  if (result && uni_strstr(result, "你好")) {
    strcpy(keyword, "你好魔方");
  } else if (result && uni_strstr(result, "我要")) {
    strcpy(keyword, "我要听歌");
  } else {
    strcpy(keyword, "N/A");
  }
  LOGD(LASR_TAG, "get original result[%s]", NULL == result ? "N/A" : result);
}

static void _engine_restart(int is_std_wakeup) {
  const char *recogn_domains = "ivm";
  const char *wakeup_domains = "wakeup";
  char *result = NULL;
  if (0 != stop()) {
    LOGE(LASR_TAG, "engine stop failed");
  }
  if (NULL != (result = getResult())) {
    LOGD(LASR_TAG, "discard result=%s", result);
  }
  if (is_std_wakeup) {
    LOGD(LASR_TAG, "engine mode std_wakeup");
  } else {
    LOGD(LASR_TAG, "engine mode std_asr");
  }
  if (0 != start(is_std_wakeup ? wakeup_domains : recogn_domains, 53)) {
    LOGE(LASR_TAG, "engine start failed");
  }
}

static unsigned int _get_current_frame_id(char *audio, int bytes_len) {
  unsigned int frame_id;
  memcpy(&frame_id, &audio[bytes_len], sizeof(unsigned int));
  return frame_id;
}

static void _set_asr_start_frame_id(char *audio, int bytes_len) {
  if (TIMESTAMP_INVALID == g_lasr.asr_start_frame_id) {
    g_lasr.asr_start_frame_id = _get_current_frame_id(audio, bytes_len);
    LOGT(LASR_TAG, "set asr start frame_id=%d", g_lasr.asr_start_frame_id);
  }
}

static void _reset_asr_start_frame_id() {
  g_lasr.asr_start_frame_id = TIMESTAMP_INVALID;
}

static unsigned int _get_timems_len(unsigned int *engine_asr_start_time,
                                    unsigned int *engine_asr_end_time) {
  *engine_asr_start_time = getOptionInt(ASR_ENGINE_UTTERANCE_START_TIME);
  *engine_asr_end_time = getOptionInt(ASR_ENGINE_UTTERANCE_STOP_TIME);
  LOGT(LASR_TAG, "engine asr time[%u-%u], len=%u", *engine_asr_start_time,
       *engine_asr_end_time, *engine_asr_end_time - *engine_asr_start_time);
  return (*engine_asr_end_time - *engine_asr_start_time);
}

static void _calc_doa_proccess(char *audio, int bytes_len) {
  unsigned int engine_asr_start_time = 0;
  unsigned int engine_asr_end_time = 0;
  unsigned int timems_len = 0;
  if (g_lasr.need_calc_doa) {
    LOGT(LASR_TAG, "need cal doa");
    timems_len = _get_timems_len(&engine_asr_start_time, &engine_asr_end_time);
    LOGD(LASR_TAG, "timems_len=%u, end_ms=%u", timems_len,
         g_lasr.asr_start_frame_id * 16 + engine_asr_end_time);
    DspGetDoa(timems_len, g_lasr.asr_start_frame_id * 16 + engine_asr_end_time);
    _set_calc_doa_status(FALSE);
  }
}

static void _set_wakeup_status(int is_wakeup) {
  g_lasr.is_wakeup = is_wakeup;
  LOGT(LASR_TAG, "switch to wakeup[%d]", is_wakeup);
}

extern long long get_vad_state();
static int _get_vad_status() {
  int vad_end = FALSE;
  long long output_vad_state_info = get_vad_state();
  while (output_vad_state_info != 0) {
    long long output_count_filter = 15ULL << 58;
    long long vad_state_filter = 3ULL << 56;
    long long time_point_filter = (1ULL << 56) - 1;
    int output_vad_state_count =
      (output_vad_state_info & output_count_filter) >> 58;
    int vad_state = (output_vad_state_info & vad_state_filter) >> 56;
    long long time_point = output_vad_state_info & time_point_filter;
    if (vad_state == 3) {
      vad_end = TRUE;
    }
    output_vad_state_info = get_vad_state();
  }
  return vad_end;
}

static void _get_keyword_frame_id_range(int *keyword_start_frame_id,
                                        int *keyword_end_frame_id) {
  int engine_asr_start_time = getOptionInt(ASR_ENGINE_UTTERANCE_START_TIME);
  int engine_asr_end_time = getOptionInt(ASR_ENGINE_UTTERANCE_STOP_TIME);
  *keyword_start_frame_id = engine_asr_start_time / 16 +
                            g_lasr.asr_start_frame_id;
  *keyword_end_frame_id = engine_asr_end_time / 16 + g_lasr.asr_start_frame_id;
  LOGT(LASR_TAG, "keyword frame_id[%d-%d], %dms, engine_time[%d-%d], "
      "asr_start_frame_id=%d", *keyword_start_frame_id, *keyword_end_frame_id,
      engine_asr_end_time - engine_asr_start_time, engine_asr_start_time,
      engine_asr_end_time, g_lasr.asr_start_frame_id);
}

static void _update_frame_range_first_id(int step_cnt) {
  g_lasr.cache_frame_range.start_frame_id += step_cnt;
}

static void _set_frame_range_latest_id(unsigned int frame_id) {
  g_lasr.cache_frame_range.latest_frame_id = frame_id;
}

static int _get_keyword_audio_source() {
  int i;
  char buf[DSP_FRAME_CNT * sizeof(short)];
  int start_frame_id;
  int end_frame_id;
  int keyword_frame_cnt;
  int skip_cnts;
  start_frame_id = uni_max(g_lasr.slice_param.keyword_start_frame_id,
                           g_lasr.cache_frame_range.start_frame_id);
  end_frame_id = uni_min(g_lasr.slice_param.keyword_end_frame_id,
                         g_lasr.cache_frame_range.latest_frame_id);
  keyword_frame_cnt = end_frame_id - start_frame_id;
  skip_cnts = start_frame_id - g_lasr.cache_frame_range.start_frame_id;
  LOGT(LASR_TAG, "keyword range[%d-%d], skip_frame=%d, cache "
       "range[%d-%d], slice range[%d-%d]", start_frame_id,
       end_frame_id, skip_cnts, g_lasr.cache_frame_range.start_frame_id,
       g_lasr.cache_frame_range.latest_frame_id,
       g_lasr.slice_param.keyword_start_frame_id,
       g_lasr.slice_param.keyword_end_frame_id);
  if (end_frame_id <= start_frame_id) {
    LOGE(LASR_TAG, "end frame < start_frame_id, fatal error");
    return -1;
  }
  for (i = 0; i < skip_cnts; i++) {
    DataBufferRead(buf, DSP_FRAME_CNT * sizeof(short), g_lasr.cache_databuf);
  }
  _update_frame_range_first_id(skip_cnts);
  g_lasr.slice_param.lasr_result.audio_len = keyword_frame_cnt * DSP_FRAME_CNT *
                                             sizeof(short);
  g_lasr.slice_param.lasr_result.audio_contain_keyword = (char *)malloc(
      g_lasr.slice_param.lasr_result.audio_len);
  DataBufferRead(g_lasr.slice_param.lasr_result.audio_contain_keyword,
                 keyword_frame_cnt * DSP_FRAME_CNT * sizeof(short),
                 g_lasr.cache_databuf);
  _update_frame_range_first_id(keyword_frame_cnt);
#if RECORD_DEBUG_OPEN
  _record(g_lasr.slice_param.lasr_result.audio_contain_keyword,
          keyword_frame_cnt * DSP_FRAME_CNT * sizeof(short));
#endif
  return 0;
}

static int _fill_slice_param(SliceParam *slice_param, char *keyword) {
  strcpy(slice_param->lasr_result.keyword, keyword);
  _get_keyword_frame_id_range(&slice_param->keyword_start_frame_id,
                              &slice_param->keyword_end_frame_id);
  if (0 == _get_keyword_audio_source()) {
    slice_param->keyword_detected = TRUE;
    return 0;
  }
  return -1;
}

static void _engine_recognize(char *raw_audio, int bytes_len,
                              SliceParam *slice_param) {
  int lasr_rc = 0;
  float score = -100.0;
  int speech_end;
  char keyword[KEY_WORD_MAX_LEN];
  pthread_mutex_lock(&g_lasr.mutex);
  _set_asr_start_frame_id(raw_audio, bytes_len);
  lasr_rc = recognize(raw_audio, bytes_len);
  if (TRUE == (speech_end = _get_vad_status())) {
    slice_param->vad_end = speech_end;
  }
  if (ASR_RECOGNIZER_PARTIAL_RESULT != lasr_rc) {
    goto L_UNLOCK;
  }
  _recognize_result_parse(getResult(), keyword, &score);
  if (LASR_WAKEUP_THRESHOLD < score) {
    if (0 != _fill_slice_param(slice_param, keyword)) {
      goto L_UNLOCK;
    }
    _set_wakeup_status(TRUE);
    DspSetEngineWakeupState(ENGINE_IS_WAKEDUP);
    _calc_doa_proccess(raw_audio, bytes_len);
    _engine_restart(FALSE);
  }
  _reset_asr_start_frame_id();
L_UNLOCK:
  pthread_mutex_unlock(&g_lasr.mutex);
}

static void _callback_register(CbAudioSource cb_audio_source) {
  g_lasr.cb_audio_source = cb_audio_source;
}

static void _callback_unregister() {
  g_lasr.cb_audio_source = NULL;
}

static void _set_frame_range_first_id(char *audio, int len) {
  if (0 != g_lasr.cache_frame_range.start_frame_id) {
    return;
  }
  g_lasr.cache_frame_range.start_frame_id = _get_current_frame_id(audio, len);
}

static void _cache_audio_source(char *audio, int len) {
  int free_size;
  char buf[PER_SOURCE_FRAME_LEN];
  _set_frame_range_first_id(audio, len);
  free_size = DataBufferGetFreeSize(g_lasr.cache_databuf);
  if (free_size < len) {
    DataBufferRead(buf, PER_SOURCE_FRAME_LEN - FRAME_ID_LEN,
                   g_lasr.cache_databuf);
    _update_frame_range_first_id(1);
  }
  DataBufferWrite(g_lasr.cache_databuf, audio, len);
  _set_frame_range_latest_id(_get_current_frame_id(audio, len));
}

static void _reset() {
  if (g_lasr.slice_param.keyword_detected) {
    free(g_lasr.slice_param.lasr_result.audio_contain_keyword);
    g_lasr.slice_param.lasr_result.audio_contain_keyword = NULL;
    g_lasr.slice_param.keyword_detected = FALSE;
  }
  g_lasr.slice_param.vad_end = FALSE;
}

#if RECORD_DEBUG_OPEN
static void _record(char *buf, int len) {
  static int fd = -1;
  if (-1 == fd ) {
    fd = uni_open("keyword.pcm", UNI_O_WRONLY | UNI_O_CREAT, 0644);
  }
  uni_write(fd, buf, len);
}
#endif

static void _slice(char *audio, int len) {
  char buf[SLICE_DATA_LEN];
  DataBufferWrite(g_lasr.slice_databuf, audio, len - FRAME_ID_LEN);
  if (g_lasr.lasr_param.frame_size_msec * 32 <=
      DataBufferGetDataSize(g_lasr.slice_databuf)) {
    DataBufferRead(buf, g_lasr.lasr_param.frame_size_msec * 32,
                   g_lasr.slice_databuf);
    g_lasr.cb_audio_source(buf, g_lasr.lasr_param.frame_size_msec * 32,
                           g_lasr.slice_param.keyword_detected ?
                           &g_lasr.slice_param.lasr_result : NULL,
                           g_lasr.slice_param.vad_end);
    _reset();
  }
}

static void _lasr_audio_source_data(char *audio, int len) {
  _cache_audio_source(audio, len - FRAME_ID_LEN);
  _engine_recognize(audio, len - FRAME_ID_LEN, &g_lasr.slice_param);
  _slice(audio, len - FRAME_ID_LEN);
}

static void _pthread_mutex_init() {
  pthread_mutex_init(&g_lasr.mutex, NULL);
}

static void _pthread_mutex_destroy() {
  pthread_mutex_destroy(&g_lasr.mutex);
}

static void _databuf_init() {
  g_lasr.cache_databuf = DataBufferCreate(CACHE_DATA_LEN);
  g_lasr.slice_databuf = DataBufferCreate(SLICE_DATA_LEN);
}

static void _databuf_destroy() {
  DataBufferDestroy(g_lasr.cache_databuf);
  DataBufferDestroy(g_lasr.slice_databuf);
}

static void _lasr_param_init(LasrParam *lasr_param) {
  memcpy(&g_lasr.lasr_param, lasr_param, sizeof(LasrParam));
}

static int _is_lasr_param_valid(LasrParam *lasr_param) {
  if (lasr_param->frame_size_msec < 16 ||
      lasr_param->frame_size_msec > 160) {
    LOGE(LASR_TAG, "unsupport frame size[%d], the valid range[16, 160]",
         lasr_param->frame_size_msec);
    return FALSE;
  }
  return TRUE;
}

int LasrInit(const char *resource_file_path, LasrParam *lasr_param,
             CbAudioSource cb_audio_source) {
  LogInitialize(NULL);
  if (NULL == resource_file_path ||
      NULL == lasr_param ||
      NULL == cb_audio_source ||
      !_is_lasr_param_valid(lasr_param)) {
    LOGE(LASR_TAG, "parameter invalid, resource_file_path=%p, "
         "lasr_param=%p, cb_audio_source=%p", resource_file_path, lasr_param,
         cb_audio_source);
    LogFinalize();
    return -1;
  }
  memset(&g_lasr, 0, sizeof(Lasr));
  _lasr_param_init(lasr_param);
  _databuf_init();
  _set_calc_doa_status(TRUE);
  _pthread_mutex_init();
  _callback_register(cb_audio_source);
  if (0 != DspInit(resource_file_path, _lasr_audio_source_data)) {
    LOGE(LASR_TAG, "dsp init failed");
    goto L_DSP_INIT_FAILED;
  }
  if (0 != _engine_init(resource_file_path)) {
    LOGE(LASR_TAG, "engine init failed");
    goto L_ENGINE_INIT_FAILED;
  }
  LOGT(LASR_TAG, "create lasr successfully");
  return 0;
L_ENGINE_INIT_FAILED:
  DspFinal();
L_DSP_INIT_FAILED:
  _databuf_destroy();
  _pthread_mutex_destroy();
  _callback_unregister();
  LogFinalize();
  return -1;
}

void LasrFinal(void) {
  LOGT(LASR_TAG, "destroy lasr successfully");
  DspFinal();
  LogFinalize();
  _callback_unregister();
  _pthread_mutex_destroy();
  _databuf_destroy();
}

int LasrWakeupReset(void) {
  pthread_mutex_lock(&g_lasr.mutex);
  if (g_lasr.is_wakeup) {
    DspSetEngineWakeupState(ENGINE_ISNOT_WAKEDUP);
    LOGT(LASR_TAG, "reset wakeup mode");
    _reset();
    _engine_restart(TRUE);
    _set_calc_doa_status(TRUE);
    _set_wakeup_status(FALSE);
    LOGT(LASR_TAG, "switch to std wakeup");
  }
  pthread_mutex_unlock(&g_lasr.mutex);
  return 0;
}
