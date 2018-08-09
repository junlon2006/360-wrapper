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

#ifndef UNI_LASR_H_
#define UNI_LASR_H_

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_WORD_MAX_LEN      (64)

typedef struct {
  char keyword[KEY_WORD_MAX_LEN];  /* 离线引擎识别到的离线命令词 */
  char *audio_contain_keyword;     /* 包含离线命令词的音频buf地址 */
  int  audio_len;                  /* 包含离线命令词音频数据长度 */
} LasrResult;

typedef struct {
  int frame_size_msec;  /* 回调aduio数据的帧长度，单位毫秒，如=50，
                           则每次CbAudioSource回调50ms数据，当前支持的范围为：
                           [16, 160] */
} LasrParam;

typedef void (*CbAudioSource)(char *audio, int bytes_len, /* 实时音频数据 */
                              LasrResult *result,         /* 离线识别结果，
                                                             如无结果则为NULL */
                              int vad_end);               /* VAD状态:1表示END，
                                                             否则为0 */

/**
 * 功能：初始化云知声离线识别引擎
 * 参数：resource_file_path 云知声引擎识别模型文件路经
         lasr_param 初始化配置参数
 *       cb_audio_source 云知声降噪后pcm数据源（1channel, 16K采样率，16bit）
         包含离线识别结果，VAD状态等信息
 * 返回：成功返回0，失败返回-1
 */
int LasrInit(const char *resource_file_path, LasrParam *lasr_param,
             CbAudioSource cb_audio_source);

/**
 * 功能：用于销毁云知声离线识别引擎占用的所有资源
 * 参数：void
 * 返回：void
 */
void LasrFinal(void);

/**
 * 功能：用于将云知声引擎状态设置为待唤醒状态，常用于云端处理超时后调用
 * 参数：void
 * 返回：成功返回0，失败返回-1
 */
int LasrWakeupReset(void);

#ifdef __cplusplus
}
#endif
#endif
