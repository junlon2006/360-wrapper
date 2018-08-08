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
 * Description : uni_tcp_ser.c
 * Author      : shangjinlong.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#include "uni_iot.h"
#include "uni_tcp_ser.h"
#include "uni_log.h"

#ifdef UNI_RASR_MODULE

#define TCP_SER_OK  0
#define TCP_SER_ERR -1

static uni_s32 uni_tcp_conn_timeout(uni_s32 sockfd, struct uni_timeval *timeo) {
#ifdef UNI_PLATFORM_QCOM
  qcom_tcp_conn_timeout(timeo->tv_sec);
#else
  uni_setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, timeo,
                 sizeof(struct timeval));
#endif
  return TCP_SER_OK;
}

uni_s32 socket_addr_init(struct sockaddr_in *psrv_add,
                         uni_in_addr_t uiIp, uni_u16 port) {
  psrv_add->sin_addr.s_addr = uiIp;
  psrv_add->sin_port = htons(port);
  psrv_add->sin_family = AF_INET;
  return TCP_SER_OK;
}

void* uni_socket_create(void) {
  struct conn_context *conx =
    (struct conn_context *)uni_malloc(sizeof(struct conn_context));
  if (conx == NULL) {
    return NULL;
  }
  uni_memset(conx, 0, sizeof(struct conn_context));
  return (void *)conx;
}

uni_s32 uni_socket_connect(void *pHandle, uni_in_addr_t uiIp, uni_u16 uiPort) {
  uni_s32 rep_code;
  struct uni_timeval timeo = {3, 0};
  struct conn_context *conn_conx = (struct conn_context *)pHandle;
  struct sockaddr_in *server_addr = NULL;
  uni_s32 rc;
  if (0 == uiIp || 0 == uiPort) {
    uni_printf("UNISDK error, ip or port is 0\n");
    rc = TCP_SER_ERR;
    goto err_out;
  }
  if (conn_conx == NULL) {
    rc = TCP_SER_ERR;
    goto err_out;
  }
  conn_conx->client_sock = uni_socket(AF_INET, SOCK_STREAM, 0);
  server_addr = (struct sockaddr_in *)uni_malloc(sizeof(struct sockaddr_in));
  if (server_addr == NULL) {
    uni_printf("%s malloc error\n", __func__);
    rc = TCP_SER_ERR;
    goto err_out;
  }
  uni_memset(server_addr, 0, sizeof(struct sockaddr_in));
  rep_code = socket_addr_init(server_addr, uiIp, uiPort);
  if (rep_code != 0) {
    rc = rep_code;
    goto err_out;
  }
  uni_tcp_conn_timeout(conn_conx->client_sock, &timeo);
  rc = uni_connect(conn_conx->client_sock, (struct sockaddr *)server_addr,
                   sizeof(struct sockaddr_in));
  if (rc != 0) {
    uni_printf("connect server error:%d\n",rc );
    uni_socket_close(conn_conx->client_sock);
  } else {
    uni_printf("connect server OK\n");
  }
err_out:
  if (NULL != server_addr) {
    uni_free(server_addr);
    server_addr = NULL;
  }
  return rc;
}

uni_s32 uni_socket_set_option(void *conx, const char *optionName,
                              const char *optionValue) {
  struct conn_context* conn_conx = (struct conn_context *)conx;
  uni_s32 index = 0;
  uni_s32 bufferIndex = 0;
  uni_s32 optionValueLen = strlen(optionValue);
  uni_s32 dataLenLeft = optionValueLen;
  char *escapeBuffer = uni_malloc(optionValueLen * 2);
  if (escapeBuffer == NULL) {
    uni_printf("%s malloc error\n", __func__);
    return -1;
  }
  while (dataLenLeft-- > 0) {
    if (optionValue[index] == ESCAPE_SYMBOL_EQ ||
        optionValue[index] == ESCAPE_SYMBOL_SP ||
        optionValue[index] == REPLACE_SYMBOL) {
      escapeBuffer[bufferIndex++] = REPLACE_SYMBOL;
      escapeBuffer[bufferIndex++] = optionValue[index] + 1;

    } else {
      escapeBuffer[bufferIndex++] = optionValue[index];
    }
    index ++;
  }
  uni_s32 opNameLen = uni_strlen(optionName);
  uni_s32 eqLen = 1;
  uni_s32 splitSLen = 1;
  uni_s32 expectLen = conn_conx->opvLen+opNameLen+eqLen+bufferIndex+splitSLen;
  uni_s32 result = 1;
  if (expectLen > MAX_OPTIONS_LEN) {
    result = -1;
  } else {
    strcpy(conn_conx->options+conn_conx->opvLen, optionName);
    conn_conx->opvLen += opNameLen;
    strncpy(conn_conx->options+conn_conx->opvLen, "=", 1);
    conn_conx->opvLen += 1;
    strncpy(conn_conx->options+conn_conx->opvLen, escapeBuffer, bufferIndex);
    conn_conx->opvLen += bufferIndex;
    strncpy(conn_conx->options+conn_conx->opvLen, ";", 1);
    conn_conx->opvLen += 1;
  }
  uni_free(escapeBuffer);
  escapeBuffer = NULL;
  return result;
}

uni_s32 uni_socket_send(void *conx, char service, uni_u8 *send_data,
                        uni_u32 size, char type) {
  uni_s32 ret = 0;
  uni_s32 send_retry_times = 10;
  uni_s32 i;
  if (conx == NULL) {
    uni_printf("uni_socket_send NULL ptr!\n");
    return -1;
  }
  struct conn_context *conn_conx = (struct conn_context *)conx;
  if (type == CLI_START_TYPE) {
    send_data = (uni_u8 *)conn_conx->options;
    size = (uni_u32)conn_conx->opvLen;
  }
  struct jHeader *frameHeader =
    (struct jHeader *)uni_malloc(sizeof(struct jHeader));
  if (frameHeader == NULL) {
    uni_printf("Alloc jHeader Failed...\n");
    return -1;
  }
  uni_memset(frameHeader, 0, sizeof( struct jHeader ));
  frameHeader->magic[0] = HEADER_SYMBOL_1;
  frameHeader->magic[1] = HEADER_SYMBOL_2;
  frameHeader->comm = type;
  frameHeader->service = service;
  frameHeader->length = htonl(size);
  for (i = 0; i < send_retry_times; i++) {
    ret = uni_send(conn_conx->client_sock, (char *)frameHeader,
        sizeof( struct jHeader ), 0);
    if (ret == sizeof(struct jHeader))
      break;
    else
      continue;
  }
  uni_free(frameHeader);
  frameHeader = NULL;
  if (i == send_retry_times) {
    uni_printf("Send Frameheader Failed...\n");
    ret = -90005;
    return ret;
  }
  if ((send_data != NULL) && (size > 0)) {
    ret = uni_send(conn_conx->client_sock, (char *)send_data, size, 0);
    if (ret <= 0) {
      uni_printf("send data to server fail.\n");
      ret = -90005;
      return ret;
    }
  }
  struct jTail *frameTail =
    (struct jTail *)uni_malloc(sizeof(struct jTail));
  if (frameTail == NULL) {
    uni_printf("Alloc jTail Failed...\n");
    return -1;
  }
  uni_memset(frameTail, 0, sizeof( struct jTail ));
  frameTail->magic[0] = TAIL_SYMBOL_1;
  frameTail->magic[1] = TAIL_SYMBOL_2;
  ret = uni_send(conn_conx->client_sock, (char *)frameTail,
      sizeof(struct jTail), 0);
  if (ret < 0) {
    uni_printf("Send FrameTail Failed...\n");
    uni_free(frameTail);
    frameTail = NULL;
    ret = -90005;
    return ret;
  } else if (ret !=sizeof(struct jTail)) {
    uni_printf("Send FrameTail Not't Match %d\n",
               (uni_s32)sizeof(struct jTail));
  }
  uni_free(frameTail);
  frameTail = NULL;
  return 0;
}

uni_s32 uni_socket_recvfrom(void *conx, uni_u8 *recv_data, uni_u32 capability,
                            char *respType) {
  struct conn_context *conn_conx = (struct conn_context *)conx;
  uni_s32 socket_conn = conn_conx->client_sock;
  uni_u8* szbuf = recv_data;
  uni_s32 recvLen = 0;
  struct timeval timeout;
  timeout.tv_sec = 5; /*1;*/
  timeout.tv_usec = 100000;//5000;//500000;//0;
  uni_fd_set fd_set_i2s2tcp;
  uni_s32 headerLenExpect = sizeof(struct jHeader);
  uni_s32 tailLenExpect = sizeof(struct jTail);
  uni_u8* respHeader = (uni_u8 *)uni_malloc(headerLenExpect);
  if (respHeader == NULL) return -1;
  uni_u8* respTail = (uni_u8 *)uni_malloc(tailLenExpect);
  if (respTail == NULL) {
    uni_free(respHeader);
    respHeader = NULL;
    return -1;
  }
  uni_memset(respHeader, 0, headerLenExpect);
  uni_memset(respTail, 0, tailLenExpect);
  uni_s32 status = HEADER_FINDING;
  uni_u8* recvBufferTarget = respHeader;
  uni_s32 targetIndex = 0;
  uni_s32 targetExpect = headerLenExpect;
  while(1) {
    FD_ZERO(&fd_set_i2s2tcp);
    FD_SET(socket_conn, &fd_set_i2s2tcp);
    uni_s32 ret = uni_select(socket_conn + 1, &fd_set_i2s2tcp, NULL, NULL,
        &timeout);
    if (ret > 0) {
      if (FD_ISSET(socket_conn, &fd_set_i2s2tcp)) {
        ret = uni_recv(socket_conn,
            (char *)(recvBufferTarget + targetIndex),
            targetExpect-targetIndex, 0);
        if (ret > 0) {
          targetIndex += ret;
          if (status == HEADER_FINDING) {
            if (targetIndex == headerLenExpect) {
              struct jHeader *headerP =
                (struct jHeader *)recvBufferTarget;
              uni_s32 respResultLen = ntohl(headerP->length);
              *respType = headerP->comm;
              if (respResultLen != 0) {
                recvBufferTarget = szbuf;
                targetIndex = 0;
                targetExpect = respResultLen;
                status = DATA_RECVING;
                if (targetExpect>capability) {
                  uni_printf("tag = %d cap = %d\n",
                      targetExpect, capability);
                  recvLen = -1;
                  break;
                }
              } else {
                recvBufferTarget = respTail;
                targetIndex = 0;
                targetExpect = tailLenExpect;
                status = TAIL_FINDING;
              }
            } else {
              continue;
            }
          } else if (status == DATA_RECVING) {
            if (targetIndex==targetExpect) {
              status = TAIL_FINDING;
              recvBufferTarget = respTail;
              targetExpect = tailLenExpect;
              recvLen = targetIndex;
              targetIndex = 0;
            } else {
              continue;
            }
          } else if (status == TAIL_FINDING) {
            if (targetIndex == targetExpect) {
              status = WHOLE_RESP_FOUND;
              break;
            } else {
              continue;
            }
          }
        } else if (ret <= 0) {
          uni_printf("recv from server zero data ret = %d\n", ret);
          recvLen = -1;
          break;
        }
      }
      timeout.tv_usec = 0;
    } else if (ret < 0) {
      //uni_printf("select quit, ret is %d\n",ret);
      recvLen = -90010;
      break;
    } else {
      //uni_printf("timeout, quit\n");
      recvLen = -90010;
      break;
    }
  }
  uni_free(respHeader);
  uni_free(respTail);
  respHeader = respTail = NULL;
  return recvLen;
}

uni_s32 uni_socket_destroy(void *conx) {
  if (NULL == conx) {
    return 0;
  }
  struct conn_context *conn_conx = (struct conn_context *)conx;
  if (conn_conx->client_sock > 0) {
    uni_socket_close(conn_conx->client_sock);
  }
  conn_conx->client_sock = -1;
  conn_conx->opvLen = 0;
  uni_free(conn_conx);
  conx = NULL;
  return 0;
}

#endif
