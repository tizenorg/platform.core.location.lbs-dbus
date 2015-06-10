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

#ifndef __LBS_DBUS_SERVER_H__
#define __LBS_DBUS_SERVER_H__

__BEGIN_DECLS

#include <gio/gio.h>

typedef void (*LbsDbusSetOptionsCB)(GVariant *options, const gchar *client, gpointer userdata);
typedef void (*LbsDbusShutdownCB)(gpointer userdata, gboolean *shutdown_arr);

typedef enum {
    LBS_SERVER_INTERVAL_ADD = 0,
    LBS_SERVER_INTERVAL_REMOVE,
    LBS_SERVER_INTERVAL_UPDATE,
} lbs_server_interval_manipulation_type;
typedef gboolean(*LbsDbusUpdateIntervalCB)(lbs_server_interval_manipulation_type type, const gchar *client, int method, guint interval, gpointer userdata);
typedef void (*LbsDbusRequestChangeIntervalCB)(int method, gpointer userdata);

typedef void (*LbsDbusGetNmeaCB)(int *timestamp, gchar **nmea_data, gpointer userdata);

/* for geofence callbacks */
typedef gint(*LbsGeofenceAddFenceCB)(const gchar *app_id,
                                     gint geofence_type,
                                     const gchar *name,
                                     gint direction,
                                     gdouble latitude,
                                     gdouble longitude,
                                     gdouble radius,
                                     const gchar *bssid,
                                     gpointer userdata);
typedef void (*LbsGeofenceRemoveFenceCB)(gint fence_id, const gchar *app_id, gpointer userdata);
typedef void (*LbsGeofencePauseFenceCB)(gint fence_id, const gchar *app_id, gpointer userdata);
typedef void (*LbsGeofenceResumeFenceCB)(gint fence_id, const gchar *app_id, gpointer userdata);
typedef void (*LbsGeofenceStartGeofenceCB)(const gchar *app_id, gpointer userdata);
typedef void (*LbsGeofenceStopGeofenceCB)(const gchar *app_id, gpointer userdata);
/* for gps-geofence (H/W geofence) callbacks */
typedef void (*GpsGeofenceAddFenceCB)(gint fence_id,
                                      gdouble latitude,
                                      gdouble longitude,
                                      gint radius,
                                      gint last_state,
                                      gint monitor_states,
                                      gint notification_responsiveness,
                                      gint unknown_timer,
                                      gpointer userdata);
typedef void (*GpsGeofenceDeleteFenceCB)(gint fence_id, gpointer userdata);
typedef void (*GpsGeofencePauseFenceCB)(gint fence_id, gpointer userdata);
typedef void (*GpsGeofenceResumeFenceCB)(gint fence_id, gint monitor_states, gpointer userdata);

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
                                 gint arg_method,
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
lbs_server_emit_batch_changed(lbs_server_dbus_h lbs_server,
                              gint arg_num_of_location);

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
lbs_server_emit_status_changed(lbs_server_dbus_h lbs_server, int method, gint status);

int
lbs_server_emit_geofence_status_changed(lbs_server_dbus_h lbs_server, gint status);

int
lbs_server_emit_geofence_changed(lbs_server_dbus_h lbs_server, const gchar *app_id, gint fence_id, gint fence_state);

int
lbs_server_emit_gps_geofence_status_changed(lbs_server_dbus_h lbs_server, gint status);

int
lbs_server_emit_gps_geofence_changed(lbs_server_dbus_h lbs_server, gint fence_id, gint transition, gdouble latitude, gdouble longitude, gdouble altitude, gdouble speed, gdouble bearing, gdouble hor_accuracy);

int
lbs_server_create(char *service_name,
                  char *service_path,
                  char *name,
                  char *description,
                  lbs_server_dbus_h *lbs_server,
                  LbsDbusSetOptionsCB set_options_cb,
                  LbsDbusShutdownCB shutdown_cb,
                  LbsDbusUpdateIntervalCB update_interval_cb,
                  LbsDbusRequestChangeIntervalCB request_change_interval_cb,
                  LbsDbusGetNmeaCB get_nmea_cb,
                  GpsGeofenceAddFenceCB add_hw_fence_cb,
                  GpsGeofenceDeleteFenceCB delete_hw_fence_cb,
                  GpsGeofencePauseFenceCB pause_hw_fence_cb,
                  GpsGeofenceResumeFenceCB resume_hw_fence_cb,
                  gpointer userdata);

int
lbs_server_destroy(lbs_server_dbus_h lbs_server);


__END_DECLS

#endif /* __LBS_DBUS_SERVER_H__ */

