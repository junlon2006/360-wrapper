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
 * Description : tts_sdk.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#ifndef SDK_PLAYER_TTS_SRC_TTS_SDK_H_
#define SDK_PLAYER_TTS_SRC_TTS_SDK_H_

#if defined MAKING_LIB
#define DLL_PUBLIC
#define DLL_LOCAL
#else
#if defined _WIN32 || defined __CYGWIN__
#ifdef MAKING_DLL
#ifdef __GNUC__
#define DLL_PUBLIC __attribute__((dllexport))
#else
#define DLL_PUBLIC __declspec(dllexport)
#endif
#else
#ifdef __GNUC__
#define DLL_PUBLIC __attribute__((dllimport))
#else
#define DLL_PUBLIC __declspec(dllimport)
#endif
#endif
#define DLL_LOCAL
#else
#if __GNUC__ >= 4
#define DLL_PUBLIC __attribute__ ((visibility("default")))
#define DLL_LOCAL  __attribute__ ((visibility("hidden")))
#else
#define DLL_PUBLIC
#define DLL_LOCAL
#endif
#endif
#endif

#ifdef __cplusplus
extern "C"{
#endif

typedef long long TTSHANDLE;

enum {
  USC_SUCCESS = 0,
  RECEIVING_AUDIO_DATA = 1,
  AUDIO_DATA_RECV_DONE = 2,
  SYNTH_PROCESS_ERROR = 3,
};

DLL_PUBLIC int         usc_tts_create_service(TTSHANDLE *handle);
DLL_PUBLIC int         usc_tts_create_service_ext(TTSHANDLE *handle,
                                                  const char *host,
                                                  const char *port);
DLL_PUBLIC int         usc_tts_release_service(TTSHANDLE handle);
DLL_PUBLIC int         usc_tts_start_synthesis(TTSHANDLE handle);
DLL_PUBLIC int         usc_tts_stop_synthesis(TTSHANDLE handle);
DLL_PUBLIC int         usc_tts_text_put(TTSHANDLE handle, const char *textdata,
                                        unsigned int textlen);
DLL_PUBLIC void*       usc_tts_get_result(TTSHANDLE handle,
                                          unsigned int *audioLen,
                                          int *synthStatus, int *errorCode);
DLL_PUBLIC const char* usc_tts_get_option(TTSHANDLE handle, int option_id,
                                          int *errorCode);
DLL_PUBLIC const char* usc_tts_get_version(TTSHANDLE handle, int *errorCode);
DLL_PUBLIC int         usc_tts_set_option(TTSHANDLE handle, const char *key,
                                          const char *value);
DLL_PUBLIC int         usc_tts_cancel(TTSHANDLE handle);

#ifdef __cplusplus
}
#endif
#endif
