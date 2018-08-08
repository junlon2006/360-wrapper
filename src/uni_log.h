#ifndef UTILS_UNI_LOG_H_
#define UTILS_UNI_LOG_H_

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
 * Description : uni_log.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.05.30
 *
 **********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  N_LOG_NONE = -1,

  N_LOG_ERROR,
  N_LOG_WARN,
  N_LOG_INFO,
  N_LOG_DEBUG,

  N_LOG_ALL
} LogLevel;

typedef struct {
  char     enable_time :1;
  char     enable_thread_id :1;
  char     enable_fuction_line :1;
  char     enable_sync :1;
  LogLevel set_level;
} LogConfig;

int LogInitialize(const LogConfig *config);
int LogFinalize(void);
int LogLevelSet(LogLevel level);

int LogLevelValid(LogLevel level);
int LogWrite(LogLevel level, const char *tags, char *function,
                int line, char *fmt, ...);

#define LOG(level, tag, fmt, ...) \
do { \
  if (LogLevelValid(level)) { \
     LogWrite(level, tag, (char *)__FUNCTION__, __LINE__, fmt"\n", ##__VA_ARGS__); \
  } \
} while (0);

#define LOGD(tag, fmt, ...) LOG(N_LOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define LOGT(tag, fmt, ...) LOG(N_LOG_INFO, tag, fmt, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...) LOG(N_LOG_WARN, tag, fmt, ##__VA_ARGS__)
#define LOGE(tag, fmt, ...) LOG(N_LOG_ERROR, tag, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif
