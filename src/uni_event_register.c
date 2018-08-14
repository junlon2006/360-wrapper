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
 * Description : uni_event_register.c
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.08.14
 *
 **********************************************************************/

#include "../inc/uni_event_register.h"
#include "uni_log.h"
#include "uni_iot.h"
#include "uni_types.h"

#define EVENT_REGISTER_TAG  "event_register"

typedef struct {
  CbEventType     event_type_callback;
  pthread_mutex_t mutex;
} EventRegister;

static EventRegister g_event_register = {NULL, PTHREAD_MUTEX_INITIALIZER};

int EventTypeCallbackRegister(CbEventType cb_event_type) {
  if (NULL == cb_event_type) {
    LOGE(EVENT_REGISTER_TAG, "cb_event_type=%p", cb_event_type);
    return -1;
  }
  pthread_mutex_lock(&g_event_register.mutex);
  g_event_register.event_type_callback = cb_event_type;
  pthread_mutex_unlock(&g_event_register.mutex);
  LOGT(EVENT_REGISTER_TAG, "register event type callback hook successfully");
  return 0;
}

void EventTypeCallbackUnRegister() {
  pthread_mutex_lock(&g_event_register.mutex);
  g_event_register.event_type_callback = NULL;
  pthread_mutex_unlock(&g_event_register.mutex);
}

const char *EventType2String(EventType type) {
  static const char* event_str[] = {
    [AUDIO_PLAY_TTS_END]     = "AUDIO_PLAY_TTS_END",
    [AUDIO_PLAY_TTS_ERROR]   = "AUDIO_PLAY_TTS_ERROR",
    [AUDIO_PLAY_MEDIA_END]   = "AUDIO_PLAY_MEDIA_END",
    [AUDIO_PLAY_MEDIA_ERROR] = "AUDIO_PLAY_MEDIA_ERROR",
    [AUDIO_PLAY_PCM_END]     = "AUDIO_PLAY_PCM_END",
    [AUDIO_PLAY_PCM_ERROR]   = "AUDIO_PLAY_PCM_ERROR",
  };
  if (type < 0 || type >= sizeof(event_str)/sizeof(char *)) {
    LOGE(EVENT_REGISTER_TAG, "invalid event type=%d", type);
    return "N/A";
  }
  return event_str[type];
}

int SendEvent(EventType type) {
  pthread_mutex_lock(&g_event_register.mutex);
  if (NULL != g_event_register.event_type_callback) {
    LOGD(EVENT_REGISTER_TAG, "send new event[%d]=%s", type,
         EventType2String(type));
    g_event_register.event_type_callback(type);
  }
  pthread_mutex_unlock(&g_event_register.mutex);
  return 0;
}
