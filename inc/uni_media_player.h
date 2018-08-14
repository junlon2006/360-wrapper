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
 * Description : uni_media_player.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.08.02
 *
 **********************************************************************/

#ifndef UNI_MEDIA_PLAYER_H_
#define UNI_MEDIA_PLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 功能：开始播放
 * 参数：url播放地址
 * 返回：成功返回0，失败返回-1
 */
int MediaPlay(char *url);

/**
 * 功能：暂停播放
 * 参数：void
 * 返回：成功返回0，失败返回-1
 */
int MediaPause(void);

/**
 * 功能：恢复播放
 * 参数：void
 * 返回：成功返回0，失败返回-1
 */
int MediaResume(void);

/**
 * 功能：停止播放
 * 参数：void
 * 返回：成功返回0，失败返回-1
 */
int MediaStop();

/**
 * 功能：是否处于播放状态
 * 参数：void
 * 返回：正在播放返回1，否则返回0
 */
int MediaPlayIsPlaying();

/**
 * 功能：是否处于暂停状态
 * 参数：void
 * 返回：处于暂停状态返回1，否则返回0
 */
int MediaPlayIsPause();

#ifdef __cplusplus
}
#endif
#endif
