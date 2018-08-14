#ifndef UTILS_UNI_UTILS_H_
#define UTILS_UNI_UTILS_H_

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
 * Date        : 2018.08.11
 *
 **********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "uni_iot.h"

Result UtilsInit(const char *resource_file_path);
void   UtilsFinal(void);

#ifdef __cplusplus
}
#endif
#endif
