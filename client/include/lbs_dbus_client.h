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

#ifndef __LBS_DBUS_CLIENT_H__
#define __LBS_DBUS_CLIENT_H__

__BEGIN_DECLS

#include <gio/gio.h>

#define SERVICE_NAME	"org.tizen.lbs.Providers.LbsServer"
#define SERVICE_PATH	"/org/tizen/lbs/Providers/LbsServer"

typedef enum {
    LBS_CLIENT_METHOD_GPS = 0,
    LBS_CLIENT_METHOD_NPS,
    LBS_CLIENT_METHOD_AGPS,
    LBS_CLIENT_METHOD_GEOFENCE,
} lbs_client_method_e;

typedef enum {
    LBS_CLIENT_LOCATION_CB = 0x01,
    LBS_CLIENT_LOCATION_STATUS_CB = LBS_CLIENT_LOCATION_CB << 0x01,
    LBS_CLIENT_SATELLITE_CB = LBS_CLIENT_LOCATION_CB << 0x02,
    LBS_CLIENT_NMEA_CB = LBS_CLIENT_LOCATION_CB << 0x03,
    LBS_CLIENT_BATCH_CB = LBS_CLIENT_LOCATION_CB << 0x04,
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
    LBS_CLIENT_ERROR_ACCESS_DENIED,
} lbs_client_error_e;

typedef void *lbs_client_dbus_h;

typedef void (*lbs_client_cb)(const char *sig, GVariant *param, void *user_data);

int lbs_client_create(lbs_client_method_e method, lbs_client_dbus_h *lbs_client);
int lbs_client_destroy(lbs_client_dbus_h lbs_client);

int lbs_client_start(lbs_client_dbus_h lbs_client, unsigned int interval, lbs_client_callback_e callback_type, lbs_client_cb callback, void *user_data);
int lbs_client_stop(lbs_client_dbus_h lbs_client);
int lbs_client_start_batch(lbs_client_dbus_h lbs_client, lbs_client_callback_e callback_type, lbs_client_cb callback, unsigned int batch_interval, unsigned int batch_period, void *user_data);
int lbs_client_stop_batch(lbs_client_dbus_h lbs_client);

int lbs_client_get_nmea(lbs_client_dbus_h lbs_client, int *timestamp, char **nmea);
int lbs_client_set_position_update_interval(lbs_client_dbus_h lbs_client, unsigned int interval);

__END_DECLS

#endif /* __LBS_DBUS_CLIENT_H__ */


