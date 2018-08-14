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
 * Description : uni_log.c
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.05.30
 *
 **********************************************************************/

#include "uni_log.h"
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#define UNI_LOG_PTHREAD_CNT_MAX  (128)
#define UNI_LOG_BUFFER_LEN       (1024)
#define UNI_LOG_PER_LEN_MAX      (1024)
#define UNI_LOG_INVALID_PID      (-1)
#define UNI_LOG_WRITE_WAIT_MS    (100)

typedef struct {
  pthread_t       pid;
  char            *buf;
  int             buf_len;
  int             remain_len;
  pthread_mutex_t mutex;
} LogBuffer;

typedef struct {
  LogBuffer       log_buf[UNI_LOG_PTHREAD_CNT_MAX];
  int             current_alive_thread_cnt;
  pthread_mutex_t mutex;
} LogAttrInfo;

static LogConfig   g_log_config = {1, 1, 1, 1, N_LOG_ALL};
static LogAttrInfo g_log_attr;
static int         g_running = 0;

static char *_level_tostring(LogLevel level) {
  switch (level) {
    case N_LOG_ERROR: return "[E]";
    case N_LOG_DEBUG: return "[D]";
    case N_LOG_INFO:  return "[T]";
    case N_LOG_WARN:  return "[W]";
    default:          return "[N]";
  }
}

static unsigned long _get_now_str(char *buf, int len) {
  struct timeval tv;
  time_t s;
  struct tm local, utc;
  int time_zone = 0;
  gettimeofday(&tv, NULL);
  s = tv.tv_sec;
  gmtime_r(&s, &utc);
  localtime_r(&s, &local);
  time_zone = local.tm_hour - utc.tm_hour;
  if (time_zone < -12) {
    time_zone += 24;
  } else if (12 < time_zone) {
    time_zone -= 24;
  }
#define PRId64 "lld"
  snprintf(buf, len, "%4d/%d/%02d %02d:%02d:%02d.%06"PRId64"(UTC%+d)",
      local.tm_year + 1900,
      local.tm_mon + 1,
      local.tm_mday,
      local.tm_hour,
      local.tm_min,
      local.tm_sec,
      (int64_t)tv.tv_usec,
      time_zone);
  return tv.tv_sec * 1000 + (tv.tv_usec / 1000);
}

static inline pthread_t _get_thread_id() {
  return pthread_self();
}

static inline void _get_thread_id_str(char *buf, int len) {
  pthread_t thread_id = pthread_self();
  snprintf(buf, len, "%x", (unsigned int)thread_id);
}

static inline int _fill_log_level(LogLevel level, char *buf, int len) {
  int wirte_len = 0;
  switch (level) {
    case N_LOG_DEBUG:
      wirte_len = snprintf(buf, len, "\033[0m\033[47;33m%s\033[0m ",
                           _level_tostring(N_LOG_DEBUG));
      break;
    case N_LOG_INFO:
      wirte_len = snprintf(buf, len, "\033[0m\033[42;33m%s\033[0m ",
                           _level_tostring(N_LOG_INFO));
      break;
    case N_LOG_WARN:
      wirte_len = snprintf(buf, len, "\033[0m\033[41;33m%s\033[0m ",
                           _level_tostring(N_LOG_WARN));
      break;
    case N_LOG_ERROR:
      wirte_len = snprintf(buf, len, "\033[0m\033[41;33m%s\033[0m ",
                           _level_tostring(N_LOG_ERROR));
      break;
    default:
      break;
  }
  return wirte_len;
}

static inline int _fill_tag(char *buf, int len, const char *tag) {
  return snprintf(buf, len, "<%s>", tag);
}

static inline int _fill_time(char *buf, int len) {
  char now[64];
  if (!g_log_config.enable_time) {
    return 0;
  }
  _get_now_str(now, sizeof(now));
  return snprintf(buf, len, "%s", now);
}

static inline int _fill_function_line(char *buf, int len,
                                      char *function, int line) {
  return (g_log_config.enable_fuction_line ? snprintf(buf, len, "%s:%d->",
          function, line) : 0);
}

static inline int _fill_thread_id(char *buf, int len) {
  char thread_id[32];
  if (!g_log_config.enable_thread_id) {
    return 0;
  }
  _get_thread_id_str(thread_id, sizeof(thread_id));
  return snprintf(buf, len, "%s", thread_id);
}

static inline int _fill_customer_info(char *buf, int len, char *fmt,
                                      va_list args) {
  return vsnprintf(buf, len, fmt, args);
}

static inline void _fill_line_feed(char *buf, int len, int cur_write_len) {
  if (len <= cur_write_len) {
    buf[len - 2] = '\n';
    buf[len - 1] = '\0';
  }
}

static int _log_buffer_polling() {
  int i;
  int empty_buffer_cnt = 0;
  char buf[UNI_LOG_BUFFER_LEN];
  int print = 0;
  for (i = 0; i < g_log_attr.current_alive_thread_cnt; i++) {
    if (0 < g_log_attr.log_buf[i].remain_len) {
      pthread_mutex_lock(&g_log_attr.log_buf[i].mutex);
      if (0 < g_log_attr.log_buf[i].remain_len) {
        strcpy(buf, g_log_attr.log_buf[i].buf);
        g_log_attr.log_buf[i].remain_len = 0;
        print = 1;
      }
      pthread_mutex_unlock(&g_log_attr.log_buf[i].mutex);
      if (print) {
        printf("%s\n", buf);
        print = 0;
      }
    } else if (0 == g_log_attr.log_buf[i].remain_len) {
      empty_buffer_cnt++;
    }
  }
  return (empty_buffer_cnt == g_log_attr.current_alive_thread_cnt ? 1 : 0);
}

int LogLevelValid(LogLevel level) {
  return level <= g_log_config.set_level ? 1 : 0;
}

#define _log_assemble(buf, level, tags, function, line, fmt, args) do { \
  int len = 0; \
  len += _fill_log_level(level, buf + len, sizeof(buf) - len); \
  len += _fill_time(buf + len, sizeof(buf) - len); \
  len += _fill_thread_id(buf + len, sizeof(buf) - len); \
  len += _fill_tag(buf + len, sizeof(buf) - len, tags); \
  len += _fill_function_line(buf + len, sizeof(buf) - len, function, line); \
  len += _fill_customer_info(buf + len, sizeof(buf) - len, fmt, args); \
  _fill_line_feed(buf, sizeof(buf), len); \
} while (0)

static inline int _pid_exist(pthread_t pid) {
  int i;
  for (i = 0; i < g_log_attr.current_alive_thread_cnt; i++) {
    if (g_log_attr.log_buf[i].pid == pid) {
      return 1;
    }
  }
  return 0;
}

static inline int _set_pid_info(pthread_t pid) {
  int index = g_log_attr.current_alive_thread_cnt;
  if (index == UNI_LOG_PTHREAD_CNT_MAX) {
    return -1;
  }
  g_log_attr.log_buf[index].pid = pid;
  g_log_attr.log_buf[index].buf = malloc(UNI_LOG_BUFFER_LEN);
  g_log_attr.log_buf[index].buf_len = UNI_LOG_BUFFER_LEN;
  g_log_attr.log_buf[index].remain_len = 0;
  g_log_attr.current_alive_thread_cnt++;
  return 0;
}

static inline LogBuffer* _get_pid_info(pthread_t pid) {
  int i;
  for (i = 0; i < g_log_attr.current_alive_thread_cnt; i++) {
    if (g_log_attr.log_buf[i].pid == pid) {
      return &g_log_attr.log_buf[i];
    }
  }
  return NULL;
}

static inline int _sync_write_process(LogLevel level, const char *tags,
                                      char *function, int line, char *fmt,
                                      va_list args) {
  char buf[UNI_LOG_PER_LEN_MAX];
  _log_assemble(buf, level, tags, function, line, fmt, args);
  printf("%s", buf);
  return 0;
}

static inline void _append_format_log_2_logbuffer(LogBuffer *buffer,
                                                  char *buf) {
  int new_str_len;
  if ((buffer->buf_len - buffer->remain_len - 1) <
      (new_str_len = strlen(buf))) {
    return;
  }
  pthread_mutex_lock(&buffer->mutex);
  if (0 == buffer->remain_len) {
    strcpy(buffer->buf, buf);
  } else {
    strcat(buffer->buf, buf);
  }
  buffer->remain_len += new_str_len;
  pthread_mutex_unlock(&buffer->mutex);
  return;
}

static inline int _async_write_process(LogLevel level, const char *tags,
                                       char *function, int line, char *fmt,
                                       va_list args) {
  pthread_t pid;
  LogBuffer *buffer;
  char buf[UNI_LOG_PER_LEN_MAX];
  int pid_exist;
  pid = _get_thread_id();
  pid_exist = _pid_exist(pid);
  if (!pid_exist) {
    pthread_mutex_lock(&g_log_attr.mutex);
    pid_exist = _pid_exist(pid);
    if (!pid_exist && -1 == _set_pid_info(pid)) {
      pthread_mutex_unlock(&g_log_attr.mutex);
      return -1;
    }
    pthread_mutex_unlock(&g_log_attr.mutex);
  }
  if (NULL == (buffer = _get_pid_info(pid))) {
    return -1;
  }
  _log_assemble(buf, level, tags, function, line, fmt, args);
  _append_format_log_2_logbuffer(buffer, buf);
  return 0;
}

int LogWrite(LogLevel level, const char *tags, char *function,
             int line, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  if (g_log_config.enable_sync) {
    _sync_write_process(level, tags, function, line, fmt, args);
  } else {
    _async_write_process(level, tags, function, line, fmt, args);
  }
  va_end(args);
  return 0;
}

static void _free_all() {
  int i;
  for (i = 0; i < g_log_attr.current_alive_thread_cnt; i++) {
    free(g_log_attr.log_buf[i].buf);
    pthread_mutex_destroy(&g_log_attr.log_buf[i].mutex);
  }
  pthread_mutex_destroy(&g_log_attr.mutex);
}

static void *_write_log_tsk(void *arg) {
  int buf_empty;
  pthread_detach(pthread_self());
  while (g_running) {
    buf_empty = _log_buffer_polling();
    if (!buf_empty) {
      continue;
    }
    usleep(UNI_LOG_WRITE_WAIT_MS * 1000);
  }
  _free_all();
  return NULL;
}

static void _create_write_log_tsk() {
  pthread_t pid;
  int i;
  memset(&g_log_attr, 0x0, sizeof(g_log_attr));
  for (i = 0; i < UNI_LOG_PTHREAD_CNT_MAX; i++) {
    g_log_attr.log_buf[i].pid = UNI_LOG_INVALID_PID;
    pthread_mutex_init(&g_log_attr.log_buf[i].mutex, NULL);
  }
  pthread_mutex_init(&g_log_attr.mutex, NULL);
  g_running = 1;
  pthread_create(&pid, NULL, _write_log_tsk, NULL);
}

int LogLevelSet(LogLevel level) {
  g_log_config.set_level = level;
  return 0;
}

static void _init_log_config(const LogConfig *config) {
  g_log_config.enable_time = config->enable_time;
  g_log_config.enable_thread_id = config->enable_thread_id;
  g_log_config.enable_fuction_line = config->enable_fuction_line;
  g_log_config.enable_sync = config->enable_sync;
  g_log_config.set_level = config->set_level;
}

int LogInitialize(const LogConfig *config) {
  if (NULL == config) {
    return 0;
  }
  _init_log_config(config);
  if (!g_log_config.enable_sync) {
    _create_write_log_tsk();
  }
  return 0;
}

int LogFinalize(void) {
  g_running = 0;
  return 0;
}
