/*
 * lbs-dbus
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Minjune Kim <sena06.kim@samsung.com>
 * 	    Genie Kim <daejins.kim@samsung.com>, Ming Zhu <mingwu.zhu@samsung.com>
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

#ifndef __LBS_DBUS_CLIENT_H__
#define __LBS_DBUS_CLIENT_H__

__BEGIN_DECLS

#include <gio/gio.h>

typedef enum {
	LBS_CLIENT_LOCATION_CB = 0x01,
	LBS_CLIENT_LOCATION_STATUS_CB = LBS_CLIENT_LOCATION_CB << 0x01,
	LBS_CLIENT_SATELLITE_CB = LBS_CLIENT_LOCATION_CB << 0x02,
	LBS_CLIENT_NMEA_CB = LBS_CLIENT_LOCATION_CB << 0x03,
} lbs_client_callback_e;

typedef enum {
	LBS_CLIENT_ERROR_NONE = 0x0,
	LBS_CLIENT_ERROR_UNKNOWN,
	LBS_CLIENT_ERROR_PARAMETER,
	LBS_CLIENT_ERROR_MEMORY,
	LBS_CLIENT_ERROR_CONNECTION,
	LBS_CLIENT_ERROR_STATUS,
	LBS_CLIENT_ERROR_DBUS_CALL,
	LBS_CLIENT_ERROR_NO_RESULT,
} lbs_client_error_e;

typedef void *lbs_client_dbus_h;

int lbs_client_create(char *service_name, char *service_path, lbs_client_dbus_h *lbs_client);
int lbs_client_destroy(lbs_client_dbus_h lbs_client);

int lbs_client_start(lbs_client_dbus_h lbs_client, lbs_client_callback_e callback_type, GDBusSignalCallback callback, void *user_data);
int lbs_client_stop(lbs_client_dbus_h lbs_client);

int lbs_client_get_nmea(lbs_client_dbus_h lbs_client, int *timestamp, char **nmea);

__END_DECLS

#endif /* __LBS_DBUS_CLIENT_H__ */


