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
 * Description : uni_config.c
 * Author      : shangjinlong.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#include "uni_config.h"
#include "uni_iot.h"
#include "cJSON.h"

#define CONFIG_ITEM_NAME_LEN_MAX   (64)

typedef enum {
  CONFIG_VALUE_NUMBER,
  CONFIG_VALUE_DOUBLE,
  CONFIG_VALUE_STRING,
  CONFIG_VALUE_ARRAY,
} ConfigValueType;

static uni_mutex_t g_mutex;

static int _config_file_exist(const char *filename) {
  return (access(filename, 0) == 0);
}

static char* _load_global_config(const char *filename) {
  int fd = -1;
  long filesize = 0;
  char *cjson_str = NULL;
  uni_pthread_mutex_lock(g_mutex);
  if ((fd = uni_open(filename, O_RDONLY)) < 0) {
    uni_pthread_mutex_unlock(g_mutex);
    return NULL;
  }
  filesize = uni_lseek(fd, 0, UNI_SEEK_END);
  if (NULL == (cjson_str = (char *)uni_malloc(filesize))) {
    goto L_ERROR;
  }
  uni_lseek(fd, 0, UNI_SEEK_SET);
  if (filesize != uni_read(fd, cjson_str, filesize)) {
    goto L_ERROR;
  }
  uni_close(fd);
  uni_pthread_mutex_unlock(g_mutex);
  return cjson_str;
L_ERROR:
  if (0 < fd) {
    uni_close(fd);
  }
  if (NULL != cjson_str) {
    uni_free(cjson_str);
  }
  uni_pthread_mutex_unlock(g_mutex);
  return NULL;
}

static char* _load_specific_config(const char *module, const char *top_cjson) {
  cJSON *father = cJSON_Parse(top_cjson);
  cJSON *child = NULL;
  char *tmp = NULL;
  if (NULL == father) {
    return NULL;
  }
  if (NULL != (child = cJSON_GetObjectItem(father, module))) {
    tmp = cJSON_Print(child);
  }
  cJSON_Delete(father);
  return tmp;
}

static char* _config_read(const char *filename, const char *module) {
  char *global = NULL;
  char *specific = NULL;
  if (!_config_file_exist(filename)) {
    return NULL;
  }
  global = _load_global_config(filename);
  if (NULL == module) {
    return global;
  }
  if (NULL != global) {
    specific = _load_specific_config(module, global);
    uni_free(global);
  }
  return specific;
}

static int _config_write(const char *filename, const char *buf, int len) {
  int fd = -1;
  uni_pthread_mutex_lock(g_mutex);
  if ((fd = uni_open(filename, O_RDWR)) < 0) {
    uni_pthread_mutex_unlock(g_mutex);
    return -1;
  }
  if (uni_write(fd, buf, len) != len) {
    uni_close(fd);
    uni_pthread_mutex_unlock(g_mutex);
    return -1;
  }
  uni_close(fd);
  uni_pthread_mutex_unlock(g_mutex);
  return 0;
}

static int _set_write_item_object(const char *filename, const char *fmt,
                                  ConfigValueType type, void *new_value) {
  char *father = NULL;
  cJSON *root = NULL;
  cJSON *tmp = NULL;
  cJSON *child = NULL;
  char buf[CONFIG_ITEM_NAME_LEN_MAX];
  int i = 0;
  if (NULL == (father = _load_global_config(filename))) {
    return -1;
  }
  if (NULL == (tmp = root = cJSON_Parse(father))) {
    uni_free(father);
    return -1;
  }
  uni_free(father);
  while((buf[i] = *fmt++) != '\0') {
    if (buf[i] == '.') {
      buf[i] = '\0';
      i = 0;
      child = cJSON_GetObjectItem(tmp, buf);
      if (NULL == (tmp = child)) {
        goto L_ERROR;
      }
    } else i++;
  }
  if (NULL == (child = cJSON_GetObjectItem(tmp, buf))) {
    goto L_ERROR;
  }
  if (CONFIG_VALUE_NUMBER == type) {
    cJSON_SetIntValue(child, *((uni_s64 *)new_value));
  } else if (CONFIG_VALUE_DOUBLE == type) {
    cJSON_SetIntValue(child, *((double *)new_value));
  } else if (CONFIG_VALUE_STRING == type) {
    uni_free(child->valuestring);
    child->valuestring = (char *)uni_malloc(uni_strlen((char *)new_value) + 1);
    uni_strcpy(child->valuestring, (char *)new_value);
  }
  father = cJSON_Print(root);
  if (NULL != father) {
    _config_write(filename, father, uni_strlen(father) + 1);
    uni_free(father);
  }
  cJSON_Delete(root);
  return 0;
L_ERROR:
  cJSON_Delete(root);
  return -1;
}

int ConfigWriteItemNumber(const char *filename, const char *fmt,
                          uni_s64 number) {
  return _set_write_item_object(filename, fmt, CONFIG_VALUE_NUMBER, &number);
}

int ConfigWriteItemDouble(const char *filename, const char *fmt,
                          double number) {
  return _set_write_item_object(filename, fmt, CONFIG_VALUE_DOUBLE, &number);
}

int ConfigWriteItemString(const char *filename, const char *fmt,
                          char *new_str) {
  return _set_write_item_object(filename, fmt, CONFIG_VALUE_STRING, new_str);
}

static cJSON *_get_read_item_object(char *buf, const char *filename,
                                    const char *fmt) {
  int i = 0;
  cJSON *key = NULL;
  char *child = NULL;
  char *father = _config_read(filename, NULL);
  if (NULL == father) {
    return NULL;
  }
  while((buf[i] = *fmt++) != '\0') {
    if (buf[i] == '.') {
      buf[i] = '\0';
      i = 0;
      child = _load_specific_config(buf, father);
      uni_free(father);
      if (NULL == (father = child)) {
        return NULL;
      }
    } else i++;
  }
  key = cJSON_Parse(father);
  uni_free(father);
  return key;
}

static const char *_config_value_type_string(ConfigValueType type) {
  switch (type) {
    case CONFIG_VALUE_NUMBER: return "uni_s64";
    case CONFIG_VALUE_DOUBLE: return "double";
    case CONFIG_VALUE_STRING: return "string";
    case CONFIG_VALUE_ARRAY: return "array";
  }
  return "null";
}

static int _customer_input_parameter_valid(cJSON *value, ConfigValueType type) {
  if (CONFIG_VALUE_ARRAY == type) {

  } else if (value->valuestring != NULL) {
    if (CONFIG_VALUE_STRING != type) {
      printf("%s%d: In parameter type invalid. actual type string, "
             "input type %s.\n",
              __FUNCTION__, __LINE__, _config_value_type_string(type));
      return 0;
    }
  } else if (value->valuedouble == value->valueint) {
    if (CONFIG_VALUE_STRING == type) {
      printf("%s%d: In parameter type invalid. actual type uni_s64 or double, "
             "input type string.\n",
              __FUNCTION__, __LINE__);
      return 0;
    }
  } else if (value->valuedouble != value->valueint) {
    if (CONFIG_VALUE_DOUBLE != type) {
      printf("%s%d: In parameter type invalid. actual type double, "
             "input type %s.\n",
              __FUNCTION__, __LINE__, _config_value_type_string(type));
      return 0;
    }
  }
  return 1;
}

int ConfigReadItemNumber(const char *filename, const char *fmt,
                         uni_s64 *number) {
  char buf[CONFIG_ITEM_NAME_LEN_MAX];
  cJSON *key = NULL;
  cJSON *value = NULL;
  if (NULL == (key = _get_read_item_object(buf, filename, fmt))) {
    return -1;
  }
  if (NULL == (value = cJSON_GetObjectItem(key, buf))) {
    cJSON_Delete(key);
    return -1;
  }
  if (!_customer_input_parameter_valid(value, CONFIG_VALUE_NUMBER)) {
    cJSON_Delete(key);
    return -1;
  }
  *number = value->valueint;
  cJSON_Delete(key);
  return 0;
}

int ConfigReadItemDouble(const char *filename, const char *fmt,
                         double *number) {
  char buf[CONFIG_ITEM_NAME_LEN_MAX];
  cJSON *key = NULL;
  cJSON *value = NULL;
  if (NULL == (key = _get_read_item_object(buf, filename, fmt))) {
    return -1;
  }
  if (NULL == (value = cJSON_GetObjectItem(key, buf))) {
    cJSON_Delete(key);
    return -1;
  }
  if (!_customer_input_parameter_valid(value, CONFIG_VALUE_DOUBLE)) {
    cJSON_Delete(key);
    return -1;
  }
  *number = value->valuedouble;
  cJSON_Delete(key);
  return 0;
}

int ConfigReadItemString(const char *filename, const char *fmt,
                         char *dst, int len) {
  char buf[CONFIG_ITEM_NAME_LEN_MAX];
  cJSON *key = NULL;
  cJSON *value = NULL;
  if (NULL == (key = _get_read_item_object(buf, filename, fmt))) {
    return -1;
  }
  if (NULL == (value = cJSON_GetObjectItem(key, buf))) {
    cJSON_Delete(key);
    return -1;
  }
  if (!_customer_input_parameter_valid(value, CONFIG_VALUE_STRING)) {
    cJSON_Delete(key);
    return -1;
  }
  if (len < uni_strlen(value->valuestring)) {
    cJSON_Delete(key);
    return -1;
  }
  uni_strcpy(dst, value->valuestring);
  cJSON_Delete(key);
  return 0;
}

int ConfigReadItemNumberArray(const char *filename, const char *fmt,
                              uni_s64 *number, int cnt) {
  char buf[CONFIG_ITEM_NAME_LEN_MAX];
  cJSON *key = NULL;
  cJSON *value = NULL;
  int size, i;
  if (NULL == (key = _get_read_item_object(buf, filename, fmt))) {
    return -1;
  }
  if (NULL == (value = cJSON_GetObjectItem(key, buf))) {
    cJSON_Delete(key);
    return -1;
  }
  if (!_customer_input_parameter_valid(value, CONFIG_VALUE_ARRAY)) {
    cJSON_Delete(key);
    return -1;
  }
  size = cJSON_GetArraySize(value);
  for (i = 0; i < size && i < cnt; i++) {
    number[i] = cJSON_GetArrayItem(value, i)->valueint;
  }
  cJSON_Delete(key);
  return 0;
}

int ConfigReadItemDoubleArray(const char *filename, const char *fmt,
                              double *number, int cnt) {
  char buf[CONFIG_ITEM_NAME_LEN_MAX];
  cJSON *key = NULL;
  cJSON *value = NULL;
  int size, i;
  if (NULL == (key = _get_read_item_object(buf, filename, fmt))) {
    return -1;
  }
  if (NULL == (value = cJSON_GetObjectItem(key, buf))) {
    cJSON_Delete(key);
    return -1;
  }
  if (!_customer_input_parameter_valid(value, CONFIG_VALUE_ARRAY)) {
    cJSON_Delete(key);
    return -1;
  }
  size = cJSON_GetArraySize(value);
  for (i = 0; i < size && i < cnt; i++) {
    number[i] = cJSON_GetArrayItem(value, i)->valuedouble;
  }
  cJSON_Delete(key);
  return 0;
}

int ConfigReadItemstringArray(const char *filename, const char *fmt,
                              char **dst, int len) {
  char buf[CONFIG_ITEM_NAME_LEN_MAX];
  cJSON *key = NULL;
  cJSON *value = NULL;
  int size, i;
  if (NULL == (key = _get_read_item_object(buf, filename, fmt))) {
    return -1;
  }
  if (NULL == (value = cJSON_GetObjectItem(key, buf))) {
    cJSON_Delete(key);
    return -1;
  }
  if (!_customer_input_parameter_valid(value, CONFIG_VALUE_ARRAY)) {
    cJSON_Delete(key);
    return -1;
  }
  size = cJSON_GetArraySize(value);
  uni_memset(dst, 0, sizeof(char *) * len);
  for (i = 0; i < uni_min(size, len); i++) {
    dst[i] = uni_malloc(uni_strlen(cJSON_GetArrayItem(value, i)->valuestring) +
                                   1);
    uni_strcpy(dst[i], cJSON_GetArrayItem(value, i)->valuestring);
  }
  cJSON_Delete(key);
  return 0;
}

int ConfigInitialize() {
  uni_pthread_mutex_init(&g_mutex);
  return 0;
}

int ConfigFinalize() {
  uni_pthread_mutex_destroy(g_mutex);
  return 0;
}
