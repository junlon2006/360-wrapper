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
 * Description : yzsttshandle.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#ifndef SDK_PLAYER_TTS_SRC_YZSTTSHANDLE_H_
#define SDK_PLAYER_TTS_SRC_YZSTTSHANDLE_H_

#ifndef DLL_PUBLIC

#if defined MAKING_LIB
#define DLL_PUBLIC
#define DLL_LOCAL
#else
#if defined _WIN32 || defined __CYGWIN__
#ifdef MAKING_DLL
#ifdef __GNUC__
#define DLL_PUBLIC __attribute__((dllexport))
#else
#define DLL_PUBLIC \
  __declspec(      \
      dllexport)  // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define DLL_PUBLIC __attribute__((dllimport))
#else
#define DLL_PUBLIC \
  __declspec(      \
      dllimport)  // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define DLL_LOCAL
#else
#if __GNUC__ >= 4
#define DLL_PUBLIC __attribute__((visibility("default")))
#define DLL_LOCAL __attribute__((visibility("hidden")))
#else
#define DLL_PUBLIC
#define DLL_LOCAL
#endif
#endif
#endif

#endif /*ifndef DLL_PUBLIC*/

#ifdef __cplusplus
extern "C" {
#endif

DLL_PUBLIC void* yzstts_createbase(const char* frontend_path,
                                   const char* backend_path,
                                   const char* marked_path,
                                   const char* user_dict_path);
DLL_PUBLIC void yzstts_releasebase(void* basehandle);

DLL_PUBLIC void* yzstts_create(void* basehandle);
DLL_PUBLIC void yzstts_release(void* handle);

DLL_PUBLIC void* yzstts_create_singleton(const char* frontend_path,
                                         const char* backend_path,
                                         const char* marked_path,
                                         const char* user_dict_path);
DLL_PUBLIC void yzstts_release_singleton(void* handle);

DLL_PUBLIC int yzstts_set_option(void* handle, int id, const char* opt);

DLL_PUBLIC int yzstts_predict_wavlenms(void* handle);

#ifndef TTS_ONLINE
DLL_PUBLIC int yzstts_change_speaker(void* handle, const char* backend_path);
#endif

#ifdef TTS_ONLINE
DLL_PUBLIC int yzstts_set_text(void* handle, const char* text,
                               const int model_index);
#else
DLL_PUBLIC int yzstts_set_text(void* handle, const char* text);
#endif
DLL_PUBLIC unsigned int yzstts_generate_wave(void* handle, short* wavbuf,
                                             unsigned int maxSampleNum);

DLL_PUBLIC unsigned int yzstts_generate_wave2(void* handle, short* wavbuf);

DLL_PUBLIC int yzstts_generate_params(void* handle, const char* text);

DLL_PUBLIC void yzstts_cancel(void* handle);
DLL_PUBLIC const char* yzstts_getversion();

#ifdef __cplusplus
}
#endif
#endif
