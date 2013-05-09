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

#ifndef __LBS_DBUS_SERVER_H__
#define __LBS_DBUS_SERVER_H__

__BEGIN_DECLS

#include <gio/gio.h>

typedef void (*LbsDbusSetOptionsCB)(GVariant *options, gpointer userdata);
typedef void (*LbsDbusShutdownCB)(gpointer userdata);


typedef enum {
	LBS_SERVER_ERROR_NONE = 0x0,
	LBS_SERVER_ERROR_UNKNOWN,
	LBS_SERVER_ERROR_PARAMETER,
	LBS_SERVER_ERROR_MEMORY,
	LBS_SERVER_ERROR_CONNECTION,
	LBS_SERVER_ERROR_STATUS,
	LBS_SERVER_ERROR_DBUS_CALL,
	LBS_SERVER_ERROR_NO_RESULT,
} lbs_server_error_e;

typedef void *lbs_server_dbus_h;

int
lbs_server_emit_position_changed(lbs_server_dbus_h lbs_server,
				gint arg_fields,
				gint arg_timestamp,
				gdouble arg_latitude,
				gdouble arg_longitude,
				gdouble arg_altitude,
				gdouble arg_speed,
				gdouble arg_direction,
				gdouble arg_climb,
				GVariant *arg_accuracy);

int
lbs_server_emit_satellite_changed(lbs_server_dbus_h lbs_server,
			gint arg_timestamp,
			gint arg_satellite_used,
			gint arg_satellite_visible,
			GVariant *arg_used_prn,
			GVariant *arg_sat_info);

int
lbs_server_emit_nmea_changed(lbs_server_dbus_h lbs_server,
			gint arg_timestamp,
			const gchar *arg_nmea_data);

int
lbs_server_emit_status_changed(lbs_server_dbus_h lbs_server, gint status);

int
lbs_server_create(char *service_name,
			char *service_path,
			char *name,
			char *description,
			lbs_server_dbus_h *lbs_server,
			LbsDbusSetOptionsCB set_options_cb,
			LbsDbusShutdownCB shutdown_cb,
			gpointer userdata);


int
lbs_server_destroy (lbs_server_dbus_h lbs_server);


__END_DECLS

#endif /* __LBS_DBUS_SERVER_H__ */

