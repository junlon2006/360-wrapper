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
 * Description : uni_tcp_ser.h
 * Author      : shangjinlong.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/

#ifndef UTILS_TCP_SERVER_INC_UNI_TCP_SER_H_
#define UTILS_TCP_SERVER_INC_UNI_TCP_SER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uni_iot.h"

#define REPLACE_SYMBOL        '&'
#define ESCAPE_SYMBOL_EQ      '='
#define ESCAPE_SYMBOL_SP      ';'

#define HEADER_SYMBOL_1       'M'
#define HEADER_SYMBOL_2       '@'
#define TAIL_SYMBOL_1         '!'
#define TAIL_SYMBOL_2         '@'

#define ASR_SERVICE           'a'
#define TTS_SERVICE           't'
#define PING_SERVICE          'p'
#define CON_SERVICE           'c'
#define KAR_SERVICE           'k'
#define QUE_SERVICE           'q'
#define MSG_SERVICE           'm'

#define CLI_START_TYPE        '1'
#define CLI_DATA_TYPE         '2'
#define CLI_END_TYPE          '3'
#define SER_START_TYPE        '4'
#define SER_DATA_TYPE         '5'
#define SER_END_TYPE          '6'

#define HEADER_FINDING        (0)
#define DATA_RECVING          (1)
#define TAIL_FINDING          (2)
#define WHOLE_RESP_FOUND      (3)

#define OPT_SYNTH_TEXT        "synthText"
#define OPT_CLIENT_NAME       "clientName"
#define OPT_APPKEY            "appkey"
#define OPT_AUDIO_CODEC       "c"
#define OPT_DOMAIN            "modelType"
#define OPT_OWNER_ID          "owner_id"
#define OPT_DEVICE_ID         "device_id"
#define OPT_TOKEN             "token"
#define OPT_AUDIO_REQ_ID      "audioReqId"
#define OPT_FILTER_URL        "filterUrl"
#define OPT_DP_NAME           "dpName"
#define OPT_ADDITIONALSERVICE "additionalService"
#define OPT_SCENARIO          "scenario"

#define MAX_OPTIONS_LEN       (512)
#define uni_timeval           timeval
#define uni_in_addr           in_addr

typedef uni_s32               uni_in_addr_t;

typedef struct conn_context {
  uni_s32 client_sock;
  char options[MAX_OPTIONS_LEN];
  uni_s32 opvLen;
} t_conn_context;

typedef struct tcp_cb {
  struct conn_context *tcp_con;
  char *tcp_res;
} t_tcp_cb;

struct jHeader {
  char magic[2];
  uni_s32 length;
  char service;
  char comm;
};

struct jTail {
  char magic[2];
};

void *uni_socket_create(void);
uni_s32 uni_socket_connect(void *pHandle, uni_in_addr_t uiIp, uni_u16 uiPort);
uni_s32 uni_socket_set_option(void *conx, const char *optionName,
                              const char *optionValue);
uni_s32 uni_socket_send(void *conx, char service, uni_u8 *send_data,
                        uni_u32 size, char type);
uni_s32 uni_socket_recvfrom(void *conx, uni_u8 *recv_data, uni_u32 capability,
                            char *respType);
uni_s32 uni_socket_destroy(void *conx);

#ifdef __cplusplus
}
#endif
#endif
