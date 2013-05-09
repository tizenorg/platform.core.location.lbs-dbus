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

#include <stdio.h>

#include "lbs_dbus_client.h"
#include "lbs_dbus_client_priv.h"

typedef struct _lbs_client_dbus_s {
	int is_started;
	gchar *addr;
	GDBusConnection *conn;
	gchar *service_name;
	gchar *service_path;
	int loc_evt_id;
	int loc_status_evt_id;
	int sat_evt_id;
	int nmea_evt_id;
	void *user_data;
} lbs_client_dbus_s;

EXPORT_API int
lbs_client_start(lbs_client_dbus_h lbs_client, lbs_client_callback_e callback_type, GDBusSignalCallback callback, void *user_data)
{
	LBS_CLIENT_LOGD("lbs_client_start");

	g_return_val_if_fail (lbs_client, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail (callback_type, LBS_CLIENT_ERROR_PARAMETER);

	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;
	g_return_val_if_fail (handle->is_started == FALSE, LBS_CLIENT_ERROR_STATUS);

	GVariant *reg = NULL;
	GVariant *param = NULL;
	GError *error = NULL;
	GVariantBuilder *builder = NULL;
	GDBusProxy *proxy = NULL;
	gchar *signal_path = NULL;

	signal_path = g_strdup_printf ("%s/%s", handle->service_path, "SAMSUNG");
	LBS_CLIENT_LOGD("LBS signal subscribe Object Path [%s]", signal_path);

	if (callback) {
		if (callback_type & LBS_CLIENT_LOCATION_CB) {
			handle->loc_evt_id = g_dbus_connection_signal_subscribe(handle->conn,
					handle->service_name, //GPS_MANAGER_SERVICE_NAME, /* Sender */
					"org.tizen.lbs.Position", /* Interface */
					"PositionChanged",  /* Member */
					signal_path,  //GPS_MANAGER_SERVICE_PATH, /* Object */
					NULL, /* arg0 */
					G_DBUS_SIGNAL_FLAGS_NONE,
					callback, user_data,
					NULL);

			if (handle->loc_evt_id) {
				LBS_CLIENT_LOGD("Listening Position info");
			}
			else {
				LBS_CLIENT_LOGD("Fail to listen Position info");
			}
		}

		if (callback_type & LBS_CLIENT_LOCATION_STATUS_CB) {
			handle->loc_status_evt_id = g_dbus_connection_signal_subscribe(handle->conn,
					handle->service_name, //GPS_MANAGER_SERVICE_NAME, /* Sender */
					"org.tizen.lbs.Manager", /* Interface */
					"StatusChanged",  /* Member */
					handle->service_path, //GPS_MANAGER_SERVICE_PATH, /* Object */
					NULL, /* arg0 */
					G_DBUS_SIGNAL_FLAGS_NONE,
					callback, user_data,
					NULL);

			if (handle->loc_status_evt_id) {
				LBS_CLIENT_LOGD("Listening location Status");
			}
			else {
				LBS_CLIENT_LOGD("Fail to listen location Status");
			}
		}

		if (callback_type & LBS_CLIENT_SATELLITE_CB) {
			handle->sat_evt_id= g_dbus_connection_signal_subscribe(handle->conn,
					handle->service_name, //GPS_MANAGER_SERVICE_NAME, /* Sender */
					"org.tizen.lbs.Satellite", /* Interface */
					"SatelliteChanged",    /* Member */
					signal_path, //GPS_MANAGER_SERVICE_PATH, /* Object */
					NULL, /* arg0 */
					G_DBUS_SIGNAL_FLAGS_NONE,
					callback, user_data,
					NULL);

			if (handle->sat_evt_id) {
				LBS_CLIENT_LOGD("Listening satellite info");
			}
			else {
				LBS_CLIENT_LOGD("Fail to listen satellite info");
			}
		}

		if (callback_type & LBS_CLIENT_NMEA_CB) {
			handle->nmea_evt_id = g_dbus_connection_signal_subscribe(handle->conn,
					handle->service_name,  /* Sender */
					"org.tizen.lbs.Nmea",  /* Interface */
					"NmeaChanged",    /* Member */
					signal_path, //GPS_MANAGER_SERVICE_PATH, /* Object */
					NULL, /* arg0 */
					G_DBUS_SIGNAL_FLAGS_NONE,
					callback, user_data,
					NULL);

			if (handle->nmea_evt_id) {
				LBS_CLIENT_LOGD("Listening nmea info");
			}
			else {
				LBS_CLIENT_LOGD("Fail to listen nmea info");
			}
		}
	}
	g_free(signal_path);

	// Start
	LBS_CLIENT_LOGD("START: CMD-START");
	builder = g_variant_builder_new(G_VARIANT_TYPE ("a{sv}"));
	g_variant_builder_add(builder, "{sv}", "CMD", g_variant_new_string("START"));
	param = g_variant_ref_sink(g_variant_new ("(@a{sv})", g_variant_builder_end(builder)));

	proxy = g_dbus_proxy_new_sync (handle->conn, // GDBusConnection
						G_DBUS_PROXY_FLAGS_NONE, //GDbusProxyFlags
						NULL,
						handle->service_name,
						handle->service_path,
						"org.tizen.lbs.Manager",
						NULL,
						&error);

	reg = g_dbus_proxy_call_sync(proxy,
						"AddReference",
						NULL,
						G_DBUS_CALL_FLAGS_NONE,
						-1,
						NULL,
						&error);

	reg = g_dbus_proxy_call_sync(proxy,
						"SetOptions",
						param,
						G_DBUS_CALL_FLAGS_NO_AUTO_START,
						-1,
						NULL,
						&error);

	g_object_unref(proxy);
	g_variant_builder_unref (builder);
	g_variant_unref(param);

	handle->is_started = TRUE;

	return LBS_CLIENT_ERROR_NONE;
}

EXPORT_API int
lbs_client_stop(lbs_client_dbus_h lbs_client)
{
	LBS_CLIENT_LOGD("lbs_client_stop");

	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;
	g_return_val_if_fail (handle, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail (handle->is_started == TRUE, LBS_CLIENT_ERROR_STATUS);

	int ret = LBS_CLIENT_ERROR_NONE;
	GVariant *param = NULL;
	GVariant *reg = NULL;
	GError *error = NULL;
	GVariantBuilder *builder = NULL;

	//TODO: Unsubscribe
	if (handle->loc_evt_id) {
		g_dbus_connection_signal_unsubscribe(handle->conn, handle->loc_evt_id);
		handle->loc_evt_id = 0;
	}

	if (handle->sat_evt_id) {
		g_dbus_connection_signal_unsubscribe(handle->conn, handle->sat_evt_id);
		handle->sat_evt_id = 0;
	}

	if (handle->loc_status_evt_id) {
		g_dbus_connection_signal_unsubscribe(handle->conn, handle->loc_status_evt_id);
		handle->loc_status_evt_id = 0;
	}

	if (handle->nmea_evt_id) {
		g_dbus_connection_signal_unsubscribe(handle->conn, handle->nmea_evt_id);
		handle->nmea_evt_id = 0;
	}

	// Stop
	LBS_CLIENT_LOGD("STOP: CMD-STOP");
	builder = g_variant_builder_new(G_VARIANT_TYPE ("a{sv}"));
	g_variant_builder_add(builder, "{sv}", "CMD", g_variant_new_string("STOP"));
	param = g_variant_ref_sink(g_variant_new ("(@a{sv})", g_variant_builder_end(builder)));

	GDBusProxy *proxy = NULL;
	proxy = g_dbus_proxy_new_sync (handle->conn, // GDBusConnection
						G_DBUS_PROXY_FLAGS_NONE, //GDbusProxyFlags
						NULL,
						handle->service_name,
						handle->service_path,
						"org.tizen.lbs.Manager",
						NULL,
						&error);
	reg = g_dbus_proxy_call_sync(proxy,
						"SetOptions",
						param,
						G_DBUS_CALL_FLAGS_NO_AUTO_START,
						-1,
						NULL,
						&error);

	reg = g_dbus_proxy_call_sync(proxy,
						"RemoveReference",
						NULL,
						G_DBUS_CALL_FLAGS_NO_AUTO_START,
						-1,
						NULL,
						&error);

	g_object_unref(proxy);
	g_variant_builder_unref (builder);
	g_variant_unref(param);
	handle->is_started = FALSE;

	return ret;
}

EXPORT_API int
lbs_client_get_nmea(lbs_client_dbus_h lbs_client, int *timestamp, char **nmea)
{
	LBS_CLIENT_LOGD("lbs_client_nmea");
	g_return_val_if_fail (lbs_client, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail (timestamp, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail (nmea, LBS_CLIENT_ERROR_PARAMETER);

	GVariant *reg = NULL;
	GError *error = NULL;
	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;

	*timestamp = 0;
	*nmea = NULL;

	reg = g_dbus_connection_call_sync(handle->conn,
					handle->service_name, //GPS_MANAGER_SERVICE_NAME, /* Bus Name */
					handle->service_path, //GPS_MANAGER_SERVICE_PATH, /* Object path */
					"org.tizen.lbs.Nmea", /* Interface*/
					"GetNmea", /* Method name */
					NULL, /* Param */
					NULL, /* Ret */
					G_DBUS_CALL_FLAGS_NONE, /* GDBusCallFlags (G_DBUS_CALL_FLAGS_NONE/G_DBUS_CALL_FLAGS_NO_AUTO_START) */
					-1, /* timeout msec */
					NULL, /* cancellable */
					&error);

	if (error) {
		LBS_CLIENT_LOGE("Fail to get nmea. Error[%s]", error->message);
		g_error_free (error);
		return LBS_CLIENT_ERROR_DBUS_CALL;
	}

	/* Get Ret */
	if (!reg) {
		LBS_CLIENT_LOGD("There is no result");
		return LBS_CLIENT_ERROR_NO_RESULT;
	}

	LBS_CLIENT_LOGD("type : %s", g_variant_get_type(reg));
	g_variant_get (reg, "(is)", timestamp, nmea);
	LBS_CLIENT_LOGD("Get NMEA. Timestamp[%d], NMEA[%s]", *timestamp, *nmea);

	return LBS_CLIENT_ERROR_NONE;
}

static int
_client_create_connection (lbs_client_dbus_s *client)
{
	LBS_CLIENT_LOGD("create connection");
	g_return_val_if_fail (client, LBS_CLIENT_ERROR_PARAMETER);
	GError *error = NULL;

	client->addr = g_dbus_address_get_for_bus_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
	if (!client->addr) {
		LBS_CLIENT_LOGD("Fail to get addr of bus.");
		return LBS_CLIENT_ERROR_CONNECTION;
	}

	client->conn = g_dbus_connection_new_for_address_sync (client->addr,
		G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
		G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
		NULL, NULL, &error);
	if (!client->conn) {
		LBS_CLIENT_LOGD("Fail to get addr of bus.");
		g_free(client->addr);
		return LBS_CLIENT_ERROR_CONNECTION;
	}

	return LBS_CLIENT_ERROR_NONE;
}

// The reason why we seperate this from start is to support IPC for db operation between a server and a client.
EXPORT_API int
lbs_client_create(char *service_name, char *service_path, lbs_client_dbus_h *lbs_client)
{
	LBS_CLIENT_LOGD("lbs_client_create");
	g_return_val_if_fail (service_name, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail (service_path, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail (lbs_client, LBS_CLIENT_ERROR_PARAMETER);

	int ret = LBS_CLIENT_ERROR_NONE;

	lbs_client_dbus_s *client = g_new0(lbs_client_dbus_s, 1);
	g_return_val_if_fail (client, LBS_CLIENT_ERROR_MEMORY);

	ret = _client_create_connection (client);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		g_free(client);
		return ret;
	}

	client->service_name = g_strdup(service_name);
	client->service_path = g_strdup(service_path);

	*lbs_client = (lbs_client_dbus_h *)client;

	return LBS_CLIENT_ERROR_NONE;
}

EXPORT_API int
lbs_client_destroy (lbs_client_dbus_h lbs_client)
{
	LBS_CLIENT_LOGD("lbs_client_destroy");
	g_return_val_if_fail(lbs_client, LBS_CLIENT_ERROR_PARAMETER);

	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;
	g_return_val_if_fail(handle->is_started == FALSE, LBS_CLIENT_ERROR_STATUS);

	int ret = LBS_CLIENT_ERROR_NONE;
	GError *error = NULL;

	//TODO : disconnect
	if (handle->conn) {
		ret = g_dbus_connection_close_sync(handle->conn, NULL, &error);
		if (ret != TRUE) {
			LBS_CLIENT_LOGD("Fail to disconnection Dbus");
		}
		handle->conn = NULL;
	}

	if (handle->addr) g_free(handle->addr);
	if (handle->service_path) g_free(handle->service_path);
	if (handle->service_name) g_free(handle->service_name);
	g_free(handle);

	return LBS_CLIENT_ERROR_NONE;
}
