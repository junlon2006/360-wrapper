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
 * Description : uni_dsp.c
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.08.01
 *
 **********************************************************************/

#include "uni_dsp.h"
#include "uni_log.h"
#include "uni_iot.h"
#include "uni_types.h"
#include "uni_databuf.h"
#include "UniLinearMicArray.h"

#define DSP_TAG                "dsp"
#define READ_DATA_DEFAULT_LEN  (65532)
#define AUDIO_CHANNLES_NUMBER  (6)
#define AUDIO_BITS             (16)
#define AUDIO_RATE             (16000)
#define PERIOD_SIZE            (2048)
#define PERIOD_CNT             (4)
#define DATABUF_SIZE           (65532 * 8)
#define RECORD_DEBUG_OPEN      (0)

typedef struct {
  pthread_t            capture_pid;
  pthread_t            dsp_pid;
  DataBufHandle        databuf;
  CbDspAudioSourceData cb_audio_source_data;
  int                  audio_capture_running;
  int                  dsp_running;
  char                 *mic;
  char                 *ref;
  char                 *out;
  void                 *mic_handle;
  void                 *audio_handle;
  char                 *audio_buffer;
  int                  audio_frame_size;
  int                  refs_num;
  EngineStatus         is_wakeup;
  int                  need_calc_doa;
  unsigned int         capture_frame_id;
  unsigned int         start_ms_len;
  unsigned int         end_ms_timestamp;
  pthread_mutex_t      mutex;
} DspInfo;

static DspInfo g_dsp_info = {0};

static void _preproc_get_4mic_refs_data(char *buf, int len) {
  int i;
  char *tmp = buf;
  char *mic = g_dsp_info.mic;
  char *ref = g_dsp_info.ref;
  for (i = 0; i < DSP_FRAME_CNT; i++) {
    memcpy(mic, tmp, 8);
    memcpy(ref, tmp + 8, 2);
    mic += 8;
    ref += 2;
    tmp += 12;
  }
}

static void _preproc_get_4mic_refs_2ch_data(char *buf, int len) {
  int i;
  char *tmp = buf;
  char *mic = g_dsp_info.mic;
  char *ref = g_dsp_info.ref;
  for (i = 0; i < DSP_FRAME_CNT; i++) {
    memcpy(mic, tmp, 8);
    memcpy(ref, tmp + 8, 4);
    mic += 8;
    ref += 4;
    tmp += 12;
  }
}

static void *_mic_array_init(const char *resource_file_path) {
  int in_num = 0;
  int echo_num = 0;
  int aec_on = 1;
  int aec = 0;
  void *handle = NULL;
  char mic_config_path[256];
  snprintf(mic_config_path, sizeof(mic_config_path), "%s/config",
           resource_file_path);
  LOGT(DSP_TAG, "mic config file path=%s", mic_config_path);
  handle = Unisound_LinearMicArray_Init(mic_config_path);
  if (NULL == handle) {
    LOGE(DSP_TAG, "initialize 4mic failed");
    return NULL;
  }
  Unisound_LinearMicArray_Set(handle, UNI_LINEAR_MICARRAY_AEC, (void *)&aec_on);
  Unisound_LinearMicArray_Get(handle, UNI_LINEAR_MICARRAY_MICROPHONE_NUM,
                              (void *)(&in_num));
  Unisound_LinearMicArray_Get(handle, UNI_LINEAR_MICARRAY_ECHO_NUM,
                              (void *)(&echo_num));
  Unisound_LinearMicArray_Get(handle, UNI_LINEAR_MICARRAY_AEC, (void *)(&aec));
  LOGT(DSP_TAG, "mic_num=%d, echo_num=%d, aec=%d, aec_on=%d", in_num, echo_num,
       aec, aec_on);
  return handle;
}

static void _mic_array_final() {
  if (NULL != g_dsp_info.mic_handle) {
    Unisound_LinearMicArray_Release(g_dsp_info.mic_handle);
    g_dsp_info.mic_handle = NULL;
  }
}

static void _wakeup_reset() {
  static int is_last_wakeup = FALSE;
  if (g_dsp_info.is_wakeup != is_last_wakeup && g_dsp_info.is_wakeup) {
    Unisound_LinearMicArray_Reset(g_dsp_info.mic_handle);
    LOGD(DSP_TAG, "reset mic array, last_wakeup=%d, is_wakeup=%d",
         is_last_wakeup, g_dsp_info.is_wakeup);
  }
  is_last_wakeup = g_dsp_info.is_wakeup;
}

#if RECORD_DEBUG_OPEN
static void _record_echo(char *buf, int len) {
  static int fd = -1;
  if (-1 == fd ) {
    fd = uni_open("echo.pcm", UNI_O_WRONLY | UNI_O_CREAT, 0644);
  }
  uni_write(fd, buf, len);
}

static void _record_4mic(char *buf, int len) {
  static int fd = -1;
  if (-1 == fd ) {
    fd = uni_open("4mic.pcm", UNI_O_WRONLY | UNI_O_CREAT, 0644);
  }
  uni_write(fd, buf, len);
}
#endif

static char *_mic_proccess(char *audio_raw_data, int len) {
  short *asr = NULL;
  short *vad = NULL;
  int outlen = 0;
  if (2 == g_dsp_info.refs_num) {
    _preproc_get_4mic_refs_2ch_data(audio_raw_data, len);
  } else {
    _preproc_get_4mic_refs_data(audio_raw_data, len);
  }
  pthread_mutex_lock(&g_dsp_info.mutex);
  g_dsp_info.capture_frame_id++;
  _wakeup_reset();
#if RECORD_DEBUG_OPEN
  _record_echo(g_dsp_info.ref, DSP_FRAME_CNT * sizeof(short) * 2);
  _record_4mic(g_dsp_info.mic, DSP_FRAME_CNT * sizeof(short) * 4);
#endif
  Unisound_LinearMicArray_Process(g_dsp_info.mic_handle,
                                  (short *)g_dsp_info.mic,
                                  DSP_FRAME_CNT,
                                  (short *)g_dsp_info.ref,
                                  g_dsp_info.is_wakeup, &asr, &vad, &outlen);
  pthread_mutex_unlock(&g_dsp_info.mutex);
  return (char *)asr;
}

static void *_audio_capture_tsk(void *args) {
  LOGT(DSP_TAG, "start audio capture task");
  g_dsp_info.audio_capture_running = TRUE;
  while (g_dsp_info.audio_capture_running) {
    uni_hal_audio_read(g_dsp_info.audio_handle,
                       g_dsp_info.audio_buffer, READ_DATA_DEFAULT_LEN);
    if (READ_DATA_DEFAULT_LEN <= DataBufferGetFreeSize(g_dsp_info.databuf)) {
      DataBufferWrite(g_dsp_info.databuf, g_dsp_info.audio_buffer,
                      READ_DATA_DEFAULT_LEN);
      continue;
    }
    LOGW(DSP_TAG, "audio source capture databuf full");
  }
  LOGT(DSP_TAG, "audio capture task exit");
  return NULL;
}

static void _audio_config_param_init(struct hal_audio_config *audio_config) {
  audio_config->channels     = AUDIO_CHANNLES_NUMBER;
  audio_config->bits         = AUDIO_BITS;
  audio_config->rate         = AUDIO_RATE;
  audio_config->period_size  = PERIOD_SIZE;
  audio_config->period_count = PERIOD_CNT;
  audio_config->is_play      = FALSE;
  LOGT(DSP_TAG, "channel=%d, bits=%d, rate=%d, period_size=%d, period_count=%d",
       AUDIO_CHANNLES_NUMBER, AUDIO_BITS, AUDIO_RATE, PERIOD_SIZE, PERIOD_CNT);
}

static int _audio_open() {
  struct hal_audio_config audio_config;
  _audio_config_param_init(&audio_config);
  if (NULL == (g_dsp_info.audio_handle = uni_hal_audio_open(&audio_config))) {
    LOGE(DSP_TAG, "open audio failed");
    return -1;
  }
  return 0;
}

static void _audio_close() {
  uni_hal_audio_close(g_dsp_info.audio_handle);
  g_dsp_info.audio_handle = NULL;
}

static int _audio_read_buffer_init() {
  g_dsp_info.audio_frame_size = uni_hal_audio_get_frame_bytes(
      g_dsp_info.audio_handle);
  g_dsp_info.audio_frame_size *= (PERIOD_SIZE * PERIOD_CNT);
  g_dsp_info.audio_buffer = (char *)malloc(g_dsp_info.audio_frame_size);
  if (NULL == g_dsp_info.audio_buffer) {
    LOGE(DSP_TAG, "alloc memory failed");
    return -1;
  }
  return 0;
}

static void _audio_read_buffer_final() {
  if (NULL != g_dsp_info.audio_buffer) {
    free(g_dsp_info.audio_buffer);
    g_dsp_info.audio_buffer = NULL;
  }
}

static void _mic_final() {
  _mic_array_final();
  if (NULL != g_dsp_info.mic) {
    free(g_dsp_info.mic);
  }
  if (NULL != g_dsp_info.ref) {
    free(g_dsp_info.ref);
  }
  if (NULL != g_dsp_info.out) {
    free(g_dsp_info.out);
  }
}

static int _mic_init(const char *resource_file_path) {
  if (NULL == (g_dsp_info.mic_handle = _mic_array_init(resource_file_path))) {
    LOGE(DSP_TAG, "mic array init failed");
    return -1;
  }
  Unisound_LinearMicArray_Get(g_dsp_info.mic_handle,
                              UNI_LINEAR_MICARRAY_ECHO_NUM,
                              (void *)&g_dsp_info.refs_num);
  LOGD(DSP_TAG, "get refs_num=%d", g_dsp_info.refs_num);
  g_dsp_info.mic = (char *)malloc(READ_DATA_DEFAULT_LEN / 6 * 4);
  if (NULL == g_dsp_info.mic) {
    LOGE(DSP_TAG, "alloc memory failed");
    goto L_ERROR;
  }
  g_dsp_info.ref = (char *)malloc(READ_DATA_DEFAULT_LEN / 6 *
                                  g_dsp_info.refs_num);
  if (NULL == g_dsp_info.ref) {
    LOGE(DSP_TAG, "alloc memory failed");
    goto L_ERROR;
  }
  g_dsp_info.out = (char *)malloc(READ_DATA_DEFAULT_LEN / 6 * 1);
  if (NULL == g_dsp_info.out) {
    LOGE(DSP_TAG, "alloc memory failed");
    goto L_ERROR;
  }
  LOGT(DSP_TAG, "mic init successfully");
  return 0;
L_ERROR:
  _mic_final();
  return -1;
}

static int _audio_capture_worker_thread_create() {
  if (0 != pthread_create(&g_dsp_info.capture_pid, NULL, _audio_capture_tsk,
                          NULL)) {
    LOGE(DSP_TAG, "create audio capture tsk failed");
    return -1;
  }
  return 0;
}

static void _audio_capture_worker_thread_destroy() {
  if (0 != g_dsp_info.capture_pid) {
    g_dsp_info.audio_capture_running = FALSE;
    pthread_join(g_dsp_info.capture_pid, NULL);
    g_dsp_info.capture_pid = 0;
  }
}

static void *_dsp_tsk(void *args) {
  char *data_with_ts = (char *)malloc(PER_SOURCE_FRAME_LEN);
  int bytes_len = DSP_FRAME_CNT * sizeof(short) * AUDIO_CHANNLES_NUMBER;
  char *buf = (char *)malloc(bytes_len);
  g_dsp_info.dsp_running = TRUE;
  while (g_dsp_info.dsp_running) {
    if (DataBufferGetDataSize(g_dsp_info.databuf) < bytes_len) {
      usleep(5 * 1000);
      continue;
    }
    DataBufferRead(buf, bytes_len, g_dsp_info.databuf);
    memcpy(data_with_ts, _mic_proccess(buf, bytes_len),
           DSP_FRAME_CNT * sizeof(short));
    memcpy(data_with_ts + DSP_FRAME_CNT * sizeof(short),
           &g_dsp_info.capture_frame_id, FRAME_ID_LEN);
    g_dsp_info.cb_audio_source_data(data_with_ts, PER_SOURCE_FRAME_LEN);
  }
  free(buf);
  free(data_with_ts);
  LOGT(DSP_TAG, "dsp task exit");
  return NULL;
}

static int _dsp_worker_thread_create() {
  if (0 != pthread_create(&g_dsp_info.dsp_pid, NULL, _dsp_tsk, NULL)) {
    LOGE(DSP_TAG, "create dsp tsk failed");
    return -1;
  }
  return 0;
}

static void _dsp_worker_thread_destroy() {
  if (0 != g_dsp_info.dsp_pid) {
    g_dsp_info.dsp_running = FALSE;
    pthread_join(g_dsp_info.dsp_pid, NULL);
    g_dsp_info.dsp_pid = 0;
  }
}

static void _audio_capture_final() {
  _audio_capture_worker_thread_destroy();
  _dsp_worker_thread_destroy();
  _audio_close();
  _audio_read_buffer_final();
  _mic_final();
}

static int _databuf_create() {
  if (NULL == (g_dsp_info.databuf = DataBufferCreate(DATABUF_SIZE))) {
    LOGE(DSP_TAG, "create databuf failed");
    return -1;
  }
  return 0;
}

static int _audio_capture_init(const char *resource_file_path) {
  if (0 != _databuf_create()) {
    return -1;
  }
  if (0 != _audio_open()) {
    LOGE(DSP_TAG, "audio open failed");
    goto L_ERROR;
  }
  if (0 != _audio_read_buffer_init()) {
    LOGE(DSP_TAG, "audio read buffer init failed");
    goto L_ERROR;
  }
  if (0 != _mic_init(resource_file_path)) {
    LOGE(DSP_TAG, "mic init failed");
    goto L_ERROR;
  }
  if (0 != _audio_capture_worker_thread_create()) {
    LOGE(DSP_TAG, "audio capture thread create failed");
    goto L_ERROR;
  }
  if (0 != _dsp_worker_thread_create()) {
    LOGE(DSP_TAG, "dsp thread create failed");
    goto L_ERROR;
  }
  return 0;
L_ERROR:
  _audio_capture_final();
  return -1;
}

static void _audio_source_data_cb_register(CbDspAudioSourceData cb_data) {
  g_dsp_info.cb_audio_source_data = cb_data;
}

static void _audio_source_data_cb_unregister() {
  g_dsp_info.cb_audio_source_data = NULL;
}

static void _pthread_mutex_init() {
  pthread_mutex_init(&g_dsp_info.mutex, NULL);
}

static void _pthread_mutex_destroy() {
  pthread_mutex_destroy(&g_dsp_info.mutex);
}

int DspInit(const char *resource_file_path,
            CbDspAudioSourceData cb_audio_source_data) {
  if (NULL == resource_file_path || NULL == cb_audio_source_data) {
    LOGE(DSP_TAG, "parameters invalid, resource_file_path=%p, "
         "cb_audio_source_data=%p", resource_file_path, cb_audio_source_data);
    return -1;
  }
  _audio_source_data_cb_register(cb_audio_source_data);
  _pthread_mutex_init();
  if (0 != _audio_capture_init(resource_file_path)) {
    LOGE(DSP_TAG, "audio capture init failed");
    goto L_ERROR;
  }
  LOGT(DSP_TAG, "dsp init successfully");
  return 0;
L_ERROR:
  _pthread_mutex_destroy();
  return -1;
}

void DspFinal() {
  _audio_capture_final();
  _audio_source_data_cb_unregister();
  _pthread_mutex_destroy();
  LOGT(DSP_TAG, "dsp final successfully");
}

int DspSetEngineWakeupState(EngineStatus status) {
  pthread_mutex_lock(&g_dsp_info.mutex);
  g_dsp_info.is_wakeup = status;
  pthread_mutex_unlock(&g_dsp_info.mutex);
  LOGT(DSP_TAG, "set dsp wakeup state mode is_wakeup=%d", status);
  return 0;
}

int DspGetDoa(unsigned int timems_len, unsigned int end_timems) {
  unsigned int delay;
  int doa;
  pthread_mutex_lock(&g_dsp_info.mutex);
  delay = g_dsp_info.capture_frame_id * 16 - end_timems;
  Unisound_LinearMicArray_Compute_DOA(g_dsp_info.mic_handle, (float)timems_len,
                                      (float)delay);
  Unisound_LinearMicArray_Get(g_dsp_info.mic_handle,
                              UNI_LINEAR_MICARRAY_DOA_RESULT, &doa);
  pthread_mutex_unlock(&g_dsp_info.mutex);
  LOGT(DSP_TAG, "timems_len=%d, delay=%d, doa=%d", timems_len, delay, doa);
  return doa;
}
