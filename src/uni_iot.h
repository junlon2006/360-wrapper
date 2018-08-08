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
 * Description : uni_iot.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#ifndef HAL_INC_UNI_IOT_H_
#define HAL_INC_UNI_IOT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/select.h>
#include <math.h>
#include <sys/types.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysinfo.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <sys/reboot.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <semaphore.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <time.h>
#include <sched.h>
#include <linux/types.h>
#include <linux/errqueue.h>
#include <linux/sockios.h>
#include <signal.h>

#include "uni_types.h"
#include "uni_sys_hal.h"

#define EVENT_TASK            "event_task"
#define LSR_TASK              "lsr_task"
#define ASR_TASK              "asr_task"
#define PLAYBACK_TASK         "playback_task"
#define PLAYLIST_TASK         "playlist_task"
#define THPOOL_TASK           "thpool_task_"
#define UNI_CROW_VERSION      "v1.0.1-$(BUILD_ID)-$(COMMIT_ID)-$(DATE)"

typedef enum {
  UNI_OPEN_ERR = 0x600,
  UNI_CLOSE_ERR,
  UNI_START_ERR,
  UNI_STOP_ERR,
  UNI_DEST_ERR,
  UNI_SUSP_ERR,
  UNI_THREAD_ERR,
  UNI_INVALID_HANDLE_ERR,
  UNI_INVALID_FILE_ERR,
  UNI_INVALID_SIZE_ERR,
  UNI_INVALID_FORMAT_ERR,
  UNI_INVALID_PARAM_ERR,
  UNI_NO_CMD_ERR,
  UNI_NO_MEM_ERR,
  UNI_NO_SUPPORT_ERR,
  UNI_MODULE_INIT_ERR,
  UNI_MODULE_UNINIT_ERR,
  UNI_MODULE_INIT_ALREADY_ERR,
  UNI_MODULE_BUSY_ERR,
  UNI_MODULE_NONE_ERR,
  UNI_INVALID_USERNAME_ERR,
  UNI_INVALID_PASSWORD_ERR,
  UNI_NO_PERMISION_ERR,
  UNI_GET_STATUS_ERR,
  UNI_SET_STATUS_ERR,
  UNI_ERR_NUM_END
} uni_com_err_t;

#define MEMBER_TO_STRUCT(member_ptr, \
                         type, member)   (type *)((char *)(member_ptr) - \
                                            (unsigned long)&((type *)0)->member)
#define UNI_ISSPACE(x)                   (((x) == 0x20) || ((x) > 0x8 && (x) < 0xe))
#define uni_assert                       assert
#define uni_max(a, b)                    ((a) < (b) ? (b) : (a))
#define uni_min(a, b)                    ((b) < (a) ? (b) : (a))
#define uni_printf                       printf
#define uni_sprintf                      sprintf
#define uni_perror                       perror
#define uni_sscanf                       sscanf
#define uni_snprintf                     snprintf

#define uni_malloc(size)                 uni_hal_malloc(size)
#define uni_calloc(n, size)              uni_hal_calloc(n, size)
#define uni_free(ptr)                    uni_hal_free(ptr)

#define uni_strtol(nptr, endptr, base)   uni_hal_strtol(nptr, endptr, base)
#define uni_strtof(nptr, endptr)         uni_hal_strtof(nptr, endptr)
#define uni_strtoll(nptr, endptr, base)  uni_hal_strtoll(nptr, endptr, base)
#define uni_strtod(nptr, endptr)         uni_hal_strtod(nptr, endptr)

#define UNI_SEEK_SET                     (0)
#define UNI_SEEK_CUR                     (1)
#define UNI_SEEK_END                     (2)

#define uni_usleep(times)                uni_hal_usleep(times)
#define uni_msleep(times)                uni_hal_msleep(times)
#define uni_sleep(times)                 uni_hal_sleep(times)

#define uni_open                         uni_hal_fopen
#define uni_read                         uni_hal_fread
#define uni_write                        uni_hal_fwrite
#define uni_lseek                        uni_hal_lseek
#define uni_close                        uni_hal_fclose
#define uni_unlink                       uni_hal_unlink

#define UNI_O_CREAT                      O_CREAT
#define UNI_O_WRONLY                     O_WRONLY
#define UNI_O_RDONLY                     O_RDONLY
#define UNI_O_RDWR                       O_RDWR

#define uni_memset(ptr, data, size)      uni_hal_memset(ptr, data, size)
#define uni_memcpy(dest, src, size)      uni_hal_memcpy(dest, src, size)
#define uni_strcat(dest, src)            uni_hal_strcat(dest, src)
#define uni_strncat(dest, src, size)     uni_hal_strncat(dest, src, size)
#define uni_strstr(haystack, needle)     uni_hal_strstr(haystack, needle)
#define uni_strrchr(s, c)                uni_hal_strrchr(s, c)
#define uni_strchr(s, c)                 uni_hal_strchr(s, c)
#define uni_strlen                       uni_hal_strlen
#define uni_strcpy                       uni_hal_strcpy
#define uni_strncpy                      uni_hal_strncpy
#define uni_strncmp                      uni_hal_strncpy
#define uni_strcmp                       uni_hal_strcmp

#define uni_isspace(ch)                  isspace(ch)

typedef void* uni_sem_t;

#define uni_sem_init(sem_ptrptr, value)  uni_hal_sem_init(sem_ptrptr, value)
#define uni_sem_post(sem_ptr)            uni_hal_sem_post(sem_ptr)
#define uni_sem_wait(sem_ptr)            uni_hal_sem_wait(sem_ptr)
#define uni_sem_destroy(sem_ptr)         uni_hal_sem_destroy(sem_ptr)

typedef void* uni_pthread_t;
#define uni_pthread_create(pthread_ptr,\
                           param, \
                           routine,\
                           args)         uni_hal_thread_create(pthread_ptr, \
                                                               param, \
                                                               routine, args)
#define uni_pthread_detach(th)           uni_hal_thread_detach(th)

typedef void* uni_mutex_t;
#define uni_pthread_mutex_init(uni_mutex_ptr)  uni_hal_mutex_init(uni_mutex_ptr)
#define uni_pthread_mutex_destroy(uni_mutex)   uni_hal_mutex_destroy(uni_mutex)
#define uni_pthread_mutex_lock(uni_mutex)      uni_hal_mutex_lock(uni_mutex)
#define uni_pthread_mutex_unlock(uni_mutex)    uni_hal_mutex_unlock(uni_mutex)

#define uni_socket(domain, type, protocol)     socket(domain, type, protocol)
#define uni_setsockopt(s, lvl, name, arg, len) setsockopt(s, lvl, name, arg, len)
#define uni_connect(socket, addr, addrlen)     connect(socket, addr, addrlen)
#define uni_bind(socket, addr, addrlen)        bind(socket, addr, addrlen)
#define uni_listen(socket, backlog)            listen(socket, backlog)
#define uni_accept(socket, addr, addrlen)      accept(socket, addr, addrlen)
#define uni_send(socket, buf, len, flags)      send(socket, buf, len, flags)
#define uni_sendto(socket, buf, len, \
                   flags, dest_addr, addrlen)  sendto(socket, buf, len, flags,\
                                                      dest_addr, addrlen)
#define uni_recv(socket, buf, len, flags)      recv(socket, buf, len, flags)
#define uni_recvfrom(socket, buf, len, \
                     flags, src_addr, \
                     addrlen) \                recvfrom(socket, buf, len, \
                                                        flags, src_addr, \
                                                        addrlen)
#define uni_socket_close(fd)                   close(fd)
#define uni_select                             select
#define uni_fd_set                             fd_set

/* timestamp from 1900 */
static inline uni_u32 uni_get_now_msec(void) {
  struct timeval t1;
  gettimeofday(&t1, NULL);
  return (t1.tv_sec * 1000 + t1.tv_usec/1000);
}

/* timestamp from 1970 */
static inline uni_u32 uni_get_now_msec_unix(void) {
  time_t timeval;
  time(&timeval);
  return (uni_u32)timeval;
}

static inline char* uni_itoa(uni_s32 num, char *buf, uni_s32 len) {
  uni_memset(buf, 0, len);
  snprintf(buf, len , "%d", num);
  return buf;
}


#ifdef __cplusplus
}
#endif
#endif
