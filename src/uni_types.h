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
 * Description : uni_types.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/

#ifndef HAL_INC_UNI_TYPES_H_
#define HAL_INC_UNI_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef int8_t                uni_s8;
typedef uint8_t               uni_u8;
typedef int16_t               uni_s16;
typedef uint16_t              uni_u16;
typedef int32_t               uni_s32;
typedef uint32_t              uni_u32;
typedef int64_t               uni_s64;
typedef uint64_t              uni_u64;
typedef int                   uni_bool;

#define TRUE                  (1)
#define FALSE                 (0)

#ifndef NULL
  #define NULL                ((void *)0)
#endif

typedef enum {
  E_OK,
  E_FAILED,
  E_REJECT,
  E_HOLD
} Result;

typedef struct {
  uni_s32 time_len;
  uni_u32 end_timestamp;
} uni_time_stamp_t;

typedef enum {
  UNI_PROTO_INVALID,
  UNI_PROTO_HTTP,
  UNI_PROTO_TCP_IP,
  UNI_PROTO_MAX
} uni_inter_proto_t;

typedef enum {
  UNI_CODEC_NONE,
  UNI_CODEC_RAW,
  UNI_CODEC_ADPCM,
  UNI_CODEC_OPUS,
  UNI_CODEC_SPEEX,
  UNI_CODEC_CNT
} uni_codec_t;

#ifdef __cplusplus
}
#endif
#endif
