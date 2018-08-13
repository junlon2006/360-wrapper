#ifndef UTILS_UNI_MP3_PARSE_PCM_H_
#define UTILS_UNI_MP3_PARSE_PCM_H_

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
 * Description : uni_mp3_parse_pcm.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.08.13
 *
 **********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  MP3_PARSE_OK = 0,
  MP3_PARSE_FAILED,
  MP3_PARSE_DONE,
} Mp3ParseErrorCode;

typedef void (*CbPoolPcmData)(char *pcm_data, int pcm_bytes_len,
                              Mp3ParseErrorCode rc);

int Mp3ParsePcm(const char *mp3_file_path, CbPoolPcmData cb_pool_pcm_data);

#ifdef __cplusplus
}
#endif
#endif
