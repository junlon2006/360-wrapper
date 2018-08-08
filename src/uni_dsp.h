#ifndef UNI_DSP_H_
#define UNI_DSP_H_

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
 * Description : uni_dsp.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.08.01
 *
 **********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#define DSP_FRAME_CNT          (256)
#define FRAME_ID_LEN           (sizeof(unsigned int))
#define PER_SOURCE_FRAME_LEN   (DSP_FRAME_CNT * sizeof(short) + FRAME_ID_LEN)

typedef enum {
  ENGINE_ISNOT_WAKEDUP = 0,
  ENGINE_IS_WAKEDUP
} EngineStatus;

typedef void (*CbDspAudioSourceData)(char *audio, int bytes_len);

/**
 * 功能：DSP降噪算法初始化
 * 参数：resource_file_path dsp配置文件目录
 *       cb_audio_source_data（1channel, 16K采样率, 16bit pcm）降噪后数据源回调
 * 返回：成功返回0，失败返回-1
 */
int DspInit(const char *resource_file_path,
            CbDspAudioSourceData cb_audio_source_data);

/**
 * 功能：用于设置降噪模式，在离线引擎进入唤醒状态和切换到待唤醒状态时调用
 * 参数：status 切换到待唤醒模式时传入ENGINE_ISNOT_WAKEDUP，
 *       切换到唤醒模式时传入ENGINE_IS_WAKEDUP
 * 返回：成功返回0，失败返回-1
 */
int DspSetEngineWakeupState(EngineStatus status);

/**
 * 功能：DSP降噪算法获取DOA
 * 参数：time_len 从识别命令词开始到结束之间的时长，time_delay 超时
 * 返回：DOA
 */
int DspGetDoa(unsigned int timems_len, unsigned int end_timems);

/**
 * 功能：DSP降噪算法退出，释放占用资源
 * 参数：void
 * 返回：void
 */
void DspFinal(void);

#ifdef __cplusplus
}
#endif
#endif
