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
 * Description : uni_sys_hal.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/

#ifndef HAL_INC_UNI_SYS_HAL_H_
#define HAL_INC_UNI_SYS_HAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PLATFORM_MTK7686   0x7686
#define UNI_HAL_PLATFORM   PLATFORM_MTK7686

#ifdef UNI_MTK_CHIP_7686
#define RTOS_EX_API 1
#endif

#include <unistd.h>

#if defined(RTOS_EX_API)
#include "sockets.h"
#endif

#if !defined(UNI_HAL_PLATFORM) || (0 >= UNI_HAL_PLATFORM)
#error "platform not defined ! ==> #define PLATFORM_RK3229_NDK 0x3229 or "
       "#define PLATFORM_KAR_C600 0x0600 or #define PLATFORM_MTK7686 0x7686"
#endif

#define UNUSED(_var)   (void)(_var)

#define HAL_DBG_LEVEL_MIN           0
#define HAL_DBG_LEVEL_ERR           1
#define HAL_DBG_LEVEL_WARN          2
#define HAL_DBG_LEVEL_NOTICE        3
#define HAL_DBG_LEVEL_INFO          4
#define HAL_DBG_LEVEL_DEBUG         5
#define HAL_DBG_LEVEL_MAX           6
#define HAL_DBG_LEVEL_DEFAULT       HAL_DBG_LEVEL_INFO

#define HAL_PRINT_ERR(x ...)        uni_hal_debug_print(HAL_DBG_LEVEL_ERR, \
                                                        "<ERR> " x)
#define HAL_PRINT_WARN(x ...)       uni_hal_debug_print(HAL_DBG_LEVEL_WARN, \
                                                        "<WARN> " x)
#define HAL_PRINT_NOTICE(x ...)     uni_hal_debug_print(HAL_DBG_LEVEL_NOTICE, \
                                                        "<NOTICE> " x)
#define HAL_PRINT_INFO(x ...)       uni_hal_debug_print(HAL_DBG_LEVEL_INFO, \
                                                        "<INFO> " x)
#define HAL_PRINT_DEBUG(x ...)      uni_hal_debug_print(HAL_DBG_LEVEL_DEBUG, \
                                                        "<DEBUG> " x)

/* POWER MODE */
#define HAL_POWER_MODE_MIN          0
#define HAL_POWER_MODE_SLEEP        1
#define HAL_POWER_MODE_IDLE         2
#define HAL_POWER_MODE_RUN_LOW      3
#define HAL_POWER_MODE_RUN_L1       4
#define HAL_POWER_MODE_RUN_L2       5
#define HAL_POWER_MODE_RUN_L3       6
#define HAL_POWER_MODE_RUN_HIGH     7
#define HAL_POWER_MODE_MAX          8

/* TASK PRIORITY */
#define OS_PRIORITY_IDLE            -1  //idle (lowest)
#define OS_PRIORITY_LOW             -2  //low
#define OS_PRIORITY_NORMAL          0   //normal
#define OS_PRIORITY_HIGH            1   //high
#define OS_PRIORITY_REALTIME        2   //realtime (highest)


/* audio */
struct hal_audio_config {
  unsigned int channels;
  unsigned int rate;
  unsigned int period_size;
  unsigned int period_count;
  unsigned int bits; /*16, 32*/
  int is_play;
};

/* network */
struct network_connect {
  char *ssid;     //"WiFi-Unisound-2.4G";
  char *pwd;      //"2017unisound";
  char *security; //"WPA-PSK-CCMP";
};

struct network_wifi_signal {
  int rssi_level;
  int linkspeed;
  int noise;
  int freq;
};

struct network_ping {
  char *host;     //if host is NULL, use the default host name or host ip
  int timeout;    //in second
  int size;       //ping size, from 4 to 65400, defualt 32
  int send_cnt;   //ping send count, default 4
  int recv_cnt;   //ping receive count
  int rtt;        //-1: DISCONNECT,  in ms
};

struct network_status {
  struct network_wifi_signal wifi_signal;
  struct network_ping ping;
};

/* version */
extern char* uni_hal_get_version(void);

/* audio */
extern void* uni_hal_audio_open(struct hal_audio_config *audio_config);
extern int uni_hal_audio_close(void *audio_handle);
/* read_size must be a multiple of frame_bytes */
extern int uni_hal_audio_read(void *audio_handle, char *buf, int read_size);
/* write_size must be a multiple of frame_bytes */
extern int uni_hal_audio_write(void *audio_handle, char *buf, int write_size);
extern int uni_hal_audio_start(void *audio_handle);
extern int uni_hal_audio_stop(void *audio_handle);
/* get the left frames to be played in alsa driver */
extern int uni_hal_audio_get_delay_frames(void *audio_handle);
/* valid volume range [0-100] */
extern int uni_hal_audio_get_volume(void *audio_handle, int *volume);
/* valid volume range [0-100] */
extern int uni_hal_audio_set_volume(void *audio_handle, int volume);
/* get bytes of a sample frame, pcm_frames_to_bytes(handle->pcm, 1) */
extern unsigned int uni_hal_audio_get_frame_bytes(void *audio_handle);

/* user data partition */
/* paths_max_size is the max length of paths */
extern int uni_hal_get_user_data_path(char *paths, int paths_max_size, int *count);
extern int uni_hal_get_user_data_path_free_size(char *path, long *avail_size);

/* file operation interface */
extern int uni_hal_fopen(const char *pathname, int flags, ... );
extern ssize_t uni_hal_fwrite (int fd, const void *buf, size_t count);
extern ssize_t uni_hal_fread(int fd, void *buf, size_t count);
extern int uni_hal_fclose(int fd);
extern int uni_hal_lseek(int fd,long offset,int whence);
extern int uni_hal_unlink(const char *pathname);

/* power mode interface */
/*core: [1,2,3], online: 0 or 1*/
extern int uni_hal_select_cpu_core(int core, int online);
extern int uni_hal_set_power_mode(int power_mode);

typedef int (*WIFI_EVENT_CALLBACK)(int event);

typedef int (*INTERNET_EVENT_CALLBACK)(int event);

typedef int (*SOFTAP_EVENT_CALLBACK)(int event);

/* network */
extern int uni_hal_get_network_mac(char *mac, int *size);
extern int uni_hal_get_network_ip(char *ip, int *size);
extern struct hostent* uni_hal_gethostbyname(const char *name);
extern int uni_hal_close_network(void);
extern int uni_hal_connect_to_network(struct network_connect *connect);
extern int uni_hal_get_wifi_signal(struct network_wifi_signal *wifi_signal);
extern int uni_hal_network_ping(struct network_ping *ping);
extern int uni_hal_get_network_status(struct network_status *status);
extern int uni_hal_get_available_wifi(char *ssid, char *pwd, char *security);
extern int uni_hal_set_netm_debug_level(int level);
extern int uni_hal_set_netm_monitor_enable(int enable);
extern int uni_hal_set_netm_auto_connect_enable(int enable);
extern int uni_hal_wifievent_register_callback(WIFI_EVENT_CALLBACK callback);
extern int uni_hal_internetevent_register_callback(INTERNET_EVENT_CALLBACK callback);
extern int uni_hal_softapevent_register_callback(SOFTAP_EVENT_CALLBACK callback);
//得到当前是否关联着ap  1   关联着ap  0没关联ap
extern int uni_hal_is_network_connected_ap();
extern int uni_hal_set_softap(char *ssid,char *password,char *security);

/* debug interface */
extern int uni_hal_set_debug_name(char *debug_name);
extern int uni_hal_set_debug_level(int level);
extern int uni_hal_set_debug_mode(int mode);
extern int uni_hal_debug_print(int level, const char *fmt, ...);

/* utility */
extern int uni_hal_exec_command(char *in, char *out, int outLen);

/* thread interface */
struct thread_param {
    int stack_size;    //stack_size, 0 for default
    int th_priority;
    char task_name[16];
};

extern int uni_hal_thread_create(void **thread, struct thread_param *param,
                                 void (*start_routine)(void *), void *args);
extern int uni_hal_thread_detach(void *thread);
extern int uni_hal_thread_destroy(void *thread);

/* semaphore */
extern int uni_hal_sem_init(void **sem, unsigned int value);
extern int uni_hal_sem_wait(void *sem);
extern int uni_hal_sem_post(void *sem);
extern int uni_hal_sem_destroy(void *sem);

/* mutex */
extern int uni_hal_mutex_init(void **mutex);
extern int uni_hal_mutex_lock(void *mutex);
extern int uni_hal_mutex_unlock(void *mutex);
extern int uni_hal_mutex_destroy(void *mutex);

/* string function */
extern void* uni_hal_memset(void *s, int ch, size_t n);
extern void* uni_hal_memcpy(void *dest, const void *src, size_t n);
extern char* uni_hal_strcat(char *dest, const char *src);
extern char* uni_hal_strncat(char *dest, const char *src, size_t n);
extern int uni_hal_strcmp(const char *str1, const char *str2);
extern int uni_hal_strncmp (const char *str1, const char *str2, size_t n);
extern char* uni_hal_strstr(const char *str1, const char *str2);
extern const char* uni_hal_strchr(const char *str, char c);
extern const char* uni_hal_strrchr(const char *str, char c);
extern char* uni_hal_strcpy(char *dest, const char *src);
extern char* uni_hal_strncpy(char *dest, const char *src, int n);
extern size_t uni_hal_strlen(const char *str);

/* assert */
#define uni_hal_assert(expr) {if(!(expr)) {\
                              uni_hal_assert_func(__func__, __LINE__, #expr);}}
extern void uni_hal_assert_func(const char *file, int line, const char *expr_str);

/* sleep */
extern int uni_hal_usleep(unsigned int us);
extern int uni_hal_msleep(unsigned int ms);
extern int uni_hal_sleep(unsigned int s);

/* stdlib */
void *uni_hal_malloc(size_t size);
void uni_hal_free(void *ptr);
void *uni_hal_calloc(size_t n, size_t size);
long int uni_hal_strtol(const char *nptr,char **endptr,int base);
long long int uni_hal_strtoll(const char *nptr, char **endptr, int base);
float uni_hal_strtof(const char *nptr, char **endptr);
double uni_hal_strtod(const char *nptr, char **endptr);

/* socket */
extern int uni_hal_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern int uni_hal_bind(int sockfd, const struct sockaddr *my_addr,
                        socklen_t addrlen);
extern int uni_hal_setsockopt(int sockfd, int level, int optname,
                              const void *optval, socklen_t optlen);
extern int uni_hal_close(int fd);
extern int uni_hal_connect(int sockfd, struct sockaddr *serv_addr, int addrlen);
extern int uni_hal_listen(int sockfd, int backlog);
extern ssize_t uni_hal_recv(int sockfd, void *buff, size_t nbytes, int flags);
extern ssize_t uni_hal_recvfrom(int sockfd, void *buf, size_t len,
                                unsigned int flags, struct sockaddr *from,
                                int *fromlen);
extern ssize_t uni_hal_send(int sockfd, const void *buff, size_t nbytes,
                            int flags);
extern int uni_hal_sendto(int sockfd, const void *msg, int len,
                          unsigned int flags, const struct sockaddr *to,
                          int tolen);
extern int uni_hal_socket(int domain, int type, int protocol);
extern int uni_hal_select(int maxfdp, fd_set *readfds, fd_set *writefds,
                          fd_set *errorfds, struct timeval *timeout);

/* THREAD STACK SIZE */
#define STACK_SIZE                  (1024 * 64)
#define STACK_SMALL_SIZE            STACK_SIZE * 1
#define STACK_NORMAL_SIZE           STACK_SIZE * 2
#define STACK_MIDDLE_SIZE           STACK_SIZE * 4
#define STACK_LARGE_SIZE            STACK_SIZE * 6

#if defined(RTOS_EX_API)
/* keyboard */
#define DEV_KEYBOARD_CORE           1
#define DEV_KEYBOARD_I2C            2
#define EVENT_KEY_UP                0
#define EVENT_KEY_DOWN              1

typedef struct keylib {
  int single_press_s;     /*unit:ms, minimal:10, suggest:200*/
  int single_press_e;     /*unit:ms, suggest:600*/
  int double_press_step;  /*200ms/step, suggest:3*/
  int long_press_step;    /*500ms/step, suggest:6*/
} KEYLIB;

typedef enum {
  KEY_GPIO,
  KEY_ADC0,
} KeyType_e;

typedef enum {
  KEY_INVAILD = 0,

  KEY_POWER_VOICE1 = 46,
  KEY_VOICE2 = 47,
  KEY_FAV = 102,
  KEY_CHAT = 28,
  KEY_VOLDOWN_PREV = 139,
  KEY_VOLUP_NEXT = 114,
  KEY_PLAY_WIFI = 115,
  KEY_CATALOG = 103,
  KEY_CKEY1 = 104,
  KEY_CKEY2 = 105,
} KeyID_e;

typedef enum {
  KEY_EVENT_CLICK = 1,
  KEY_EVENT_DCLICK = 2,
  KEY_EVENT_LONGPRESS = 3,
} KeyEvent_e;

typedef enum {
  KEY_STATE_INIT = 0,
  KEY_STATE_PRESS1,
  KEY_STATE_PRESS2,
  KEY_STATE_LONG_PRESS,
  KEY_STATE_RELASE1,
  KEY_STATE_RELASE2,
} KeyState_e;

/* keyboard */
typedef int (*KEYBOARD_CALLBACK)(int event, int keyCode);
extern int uni_hal_keyboard_open(int deviceid, KEYLIB parm);
extern int uni_hal_keyboard_close(void);
extern int uni_hal_keyboard_register_callback(int deviceid,
                                              KEYBOARD_CALLBACK callback);

/*audio test*/
extern int uni_hal_audio_test();

/* sflash operation interface */
extern void* uni_hal_sflash_open(void);
extern int uni_hal_sflash_close(void *sf_handle);
extern int uni_hal_sflash_read(void *sf_handle, int addr, void *buf, int size);
extern int uni_hal_sflash_write(void *sf_handle, int addr, void *buf, int size);

/* message queue */
extern int uni_hal_msg_create(void **queue_handle, int queue_size,
                              int pkg_size, void *config);
/*config uses for adapt all kinds of platform's interface*/
extern int uni_hal_msg_delete(void *queue_handle);
extern int uni_hal_msg_send(void *queue_handle, const void *buf, void *config);
extern int uni_hal_msg_recv(void *queue_handle, const void *buf, void *config);
extern int uni_hal_msg_clear(void *queue_handle);

#endif
#ifdef __cplusplus
}
#endif
#endif
