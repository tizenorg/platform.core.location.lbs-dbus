/*
 * lbs-dbus
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Minjune Kim <sena06.kim@samsung.com>
 *          Genie Kim <daejins.kim@samsung.com>, Ming Zhu <mingwu.zhu@samsung.com>
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

#ifndef __LBS_DBUS_SERVER_PRIV_H__
#define __LBS_DBUS_SERVER_PRIV_H__

__BEGIN_DECLS

#ifdef FEATURE_DLOG_DEBUG
#include <dlog.h>

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "LBS_DBUS_SERVER"
#endif

#define LBS_SERVER_LOGI(fmt,args...)  { LOGI(fmt, ##args); }
#define LBS_SERVER_LOGD(fmt,args...)  { LOGD(fmt, ##args); }
#define LBS_SERVER_LOGW(fmt,args...)  { LOGW(fmt, ##args); }
#define LBS_SERVER_LOGE(fmt,args...)  { LOGE(fmt, ##args); }

#else

#define LBS_SERVER_LOGI(fmt,args...)
#define LBS_SERVER_LOGD(fmt,args...)
#define LBS_SERVER_LOGW(fmt,args...)
#define LBS_SERVER_LOGE(fmt,args...)

#endif

__END_DECLS


#endif
