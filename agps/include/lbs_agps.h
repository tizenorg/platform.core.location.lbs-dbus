/*
 * lbs-dbus
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Minjune Kim <sena06.kim@samsung.com>
 *			Genie Kim <daejins.kim@samsung.com>, Ming Zhu <mingwu.zhu@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __LBS_AGPS_H__
#define __LBS_AGPS_H__

__BEGIN_DECLS

#include <gio/gio.h>

typedef enum {
    LBS_AGPS_ERROR_NONE = 0x0,
    LBS_AGPS_ERROR_UNKNOWN,
    LBS_AGPS_ERROR_PARAMETER,
    LBS_AGPS_ERROR_MEMORY,
    LBS_AGPS_ERROR_CONNECTION,
    LBS_AGPS_ERROR_STATUS,
    LBS_AGPS_ERROR_DBUS_CALL,
    LBS_AGPS_ERROR_NO_RESULT,
} lbs_agps_error_e;

int lbs_agps_sms(const char *msg_body, int msg_size);
int lbs_agps_wap_push(const char *push_header, const char *push_body, int push_body_size);
int lbs_set_option(const char *option);

__END_DECLS

#endif /* __LBS_AGPS_H__ */


