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
 * Description : uni_event_register.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.08.14
 *
 **********************************************************************/

#ifndef UNI_EVENT_REGISTER_H_
#define UNI_EVENT_REGISTER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AUDIO_PLAY_TTS_END = 0,
  AUDIO_PLAY_TTS_ERROR,
  AUDIO_PLAY_MEDIA_END,
  AUDIO_PLAY_MEDIA_ERROR,
  AUDIO_PLAY_PCM_END,
  AUDIO_PLAY_PCM_ERROR,
} EventType;

typedef void (*CbEventType)(EventType type);

int EventTypeCallbackRegister(CbEventType cb_event_type);
void EventTypeCallbackUnRegister();
const char *EventType2String(EventType type);

#ifdef __cplusplus
}
#endif
#endif
