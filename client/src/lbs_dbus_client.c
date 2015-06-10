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

#include <stdio.h>
#include <app_manager.h>
#include <package_manager.h>
#include "lbs_dbus_client.h"
#include "lbs_dbus_client_priv.h"
#include "generated-code.h"

#define LOCATION_PRIVACY_ID	"http://tizen.org/privacy/location"
#define PRIVACY_INTERFACE	"org.tizen.privacy_manager.signal"
#define PRIVACY_MEMBER		"privacy_setting_changed"
#define PRIVACY_PATH		"/privacy_manager/dbus_notification"

typedef struct _lbs_client_dbus_s {
	int is_started;
	GDBusConnection *conn;
	lbs_client_method_e method;
	int loc_evt_id;
	int loc_status_evt_id;
	int sat_evt_id;
	int batch_evt_id;
	int nmea_evt_id;
	int privacy_evt_id;
	lbs_client_cb user_cb;
	lbs_client_cb batch_cb;
	void *user_data;
} lbs_client_dbus_s;

static int __lbs_get_appid(char **app_id)
{
	pid_t pid = getpid();
	int ret = 0;
	char *aid = NULL;

	ret = app_manager_get_app_id(pid, &aid);
	if (ret != APP_MANAGER_ERROR_NONE || aid == NULL) {
		LBS_CLIENT_LOGE("Fail to app_manager_get_package. Error[%d]", ret);
		return FALSE;
	}
	*app_id = (char *)g_malloc0(sizeof(char) * 64);
	g_strlcpy(*app_id, aid, 64);
	LBS_CLIENT_LOGD("get_appid %s", *app_id);
	g_free(aid);

	return TRUE;
}

static int __lbs_get_app_type(char *app_id, char **type)
{
	int ret;
	app_info_h app_info;
	char *app_type = NULL;

	ret = app_info_create(app_id, &app_info);
	if (ret != APP_MANAGER_ERROR_NONE) {
		LBS_CLIENT_LOGE("Fail to get app_info. Err[%d]", ret);
		return FALSE;
	}

	ret = app_info_get_type(app_info, &app_type);
	if (ret != APP_MANAGER_ERROR_NONE) {
		LBS_CLIENT_LOGE("Fail to get type. Err[%d]", ret);
		app_info_destroy(app_info);
		return FALSE;
	}

	*type = (char *)g_malloc0(sizeof(char) * 16);
	g_strlcpy(*type, app_type, 16);
	g_free(app_type);
	app_info_destroy(app_info);

	return TRUE;
}

static int __lbs_check_package_id(char *pkg_id)
{
	int ret;
	gchar *app_id = NULL;
	gchar *type = NULL;
	char *package_id = NULL;

	if (!__lbs_get_appid(&app_id)) {
		return FALSE;
	}
	if (!__lbs_get_app_type(app_id, &type)) {
		return FALSE;
	}

	if ((g_strcmp0(type, "c++app") == 0) || (g_strcmp0(type, "webapp") == 0)) {
		LBS_CLIENT_LOGE("Do not check for App[%s] Type[%s]", app_id, type);
		g_free(app_id);
		g_free(type);
		return FALSE;
	}
	g_free(type);

	ret = package_manager_get_package_id_by_app_id(app_id, &package_id);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		LBS_CLIENT_LOGE("Fail to get package_id for [%s]. Err[%d]", app_id, ret);
		g_free(app_id);
		return FALSE;
	}
	LBS_CLIENT_LOGD("Current package[%s] / Privacy package[%s]", package_id, pkg_id);

	if (g_strcmp0(pkg_id, package_id) == 0) {
		ret = TRUE;
	} else {
		ret = FALSE;
	}
	g_free(package_id);
	g_free(app_id);

	return ret;
}


static void __signal_batch_callback(GDBusConnection *conn, const gchar *name, const gchar *path, const gchar *interface, const gchar *sig, GVariant *param, gpointer user_data)
{
	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)user_data;
	if (handle == NULL) {
		LBS_CLIENT_LOGD("Invalid handle");
		return;
	}

	if (handle->is_started == FALSE) {
		LBS_CLIENT_LOGD("Handle[%p] is not started", handle);
		return;
	}

	if (handle->batch_cb) {
		handle->batch_cb(sig, param, handle->user_data);
	}
}

static void __signal_callback(GDBusConnection *conn, const gchar *name, const gchar *path, const gchar *interface, const gchar *sig, GVariant *param, gpointer user_data)
{
	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)user_data;
	if (handle == NULL) {
		LBS_CLIENT_LOGD("Invalid handle");
		return;
	}

	if (handle->is_started == FALSE) {
		LBS_CLIENT_LOGD("Handle[%p] is not started", handle);
		return;
	}

	if (handle->user_cb) {
		handle->user_cb(sig, param, handle->user_data);
	}
}

static void __privacy_setting_changed(GDBusConnection *conn, const gchar *name, const gchar *path, const gchar *interface, const gchar *sig, GVariant *param, gpointer user_data)
{
	LBS_CLIENT_LOGD("ENTER >>>");

	gchar *pkg_id = NULL;
	gchar *privacy_id = NULL;

	g_variant_get(param, "(ss)", &pkg_id, &privacy_id);
	if (!pkg_id) {
		return;
	}
	if (!privacy_id) {
		g_free(pkg_id);
		return;
	}

	if (g_strcmp0(privacy_id, LOCATION_PRIVACY_ID) != 0) {
		LBS_CLIENT_LOGD("[%s]'s [%s] privacy is changed", pkg_id, privacy_id);
		g_free(pkg_id);
		g_free(privacy_id);
		return;
	}

	LBS_CLIENT_LOGD("[%s]'s location privacy is changed", pkg_id, privacy_id);
	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)user_data;

	if (handle == NULL || handle->is_started == FALSE) {
		LBS_CLIENT_LOGE("Invalid handle or is_started [FALSE]");
		g_free(pkg_id);
		g_free(privacy_id);
		return;
	}

	if (!__lbs_check_package_id(pkg_id)) {
		LBS_CLIENT_LOGE("pkg_id[%s] is not current pakcage id", pkg_id);
		g_free(pkg_id);
		g_free(privacy_id);
		return;
	}
	g_free(pkg_id);
	g_free(privacy_id);

	if (lbs_client_stop(handle) != LBS_CLIENT_ERROR_NONE) {
		LBS_CLIENT_LOGE("lbs_client_stop is fail");
	}

	if (handle->user_cb) {
		handle->user_cb("StatusChanged", g_variant_new("(i)", FALSE), handle->user_data);
	}

	lbs_client_destroy(handle);
	handle = NULL;

	LBS_CLIENT_LOGD("EXIT <<<");
}

static void
lbs_client_signal_unsubcribe(lbs_client_dbus_h lbs_client)
{
	LBS_CLIENT_LOGD("lbs_client_unsubcribe");

	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;
	if (handle == NULL) {
		LBS_CLIENT_LOGD("Invalid handle");
		return;
	}
	if (handle->conn == NULL) {
		LBS_CLIENT_LOGD("Invalid dbus_connection");
		return;
	}

	if (handle->loc_evt_id) {
		g_dbus_connection_signal_unsubscribe(handle->conn, handle->loc_evt_id);
		handle->loc_evt_id = 0;
	}

	if (handle->batch_evt_id) {
		g_dbus_connection_signal_unsubscribe(handle->conn, handle->batch_evt_id);
		handle->batch_evt_id = 0;
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
}

static void
lbs_client_privacy_signal_subcribe(lbs_client_dbus_h lbs_client)
{
	LBS_CLIENT_LOGD("lbs_client_privacy_signal_subcribe");

	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;
	handle->privacy_evt_id = g_dbus_connection_signal_subscribe(handle->conn, NULL,
	                                                            PRIVACY_INTERFACE,
	                                                            PRIVACY_MEMBER,
	                                                            PRIVACY_PATH,
	                                                            NULL, G_DBUS_SIGNAL_FLAGS_NONE,
	                                                            __privacy_setting_changed, handle,
	                                                            NULL);

	if (handle->privacy_evt_id) {
		LBS_CLIENT_LOGD("Listening Privacy info");
	} else {
		LBS_CLIENT_LOGD("Fail to listen Privacy info");
	}
}

static void
lbs_client_privacy_signal_unsubcribe(lbs_client_dbus_h lbs_client)
{
	LBS_CLIENT_LOGD("lbs_client_privacy_signal_unsubcribe");

	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;
	if (handle->privacy_evt_id) {
		g_dbus_connection_signal_unsubscribe(handle->conn, handle->privacy_evt_id);
		handle->privacy_evt_id = 0;
	}
}

EXPORT_API int
lbs_client_start_batch(lbs_client_dbus_h lbs_client, lbs_client_callback_e callback_type, lbs_client_cb callback, unsigned int batch_interval, unsigned int batch_period, void *user_data)
{
	LBS_CLIENT_LOGD("lbs_client_start_batch");

	g_return_val_if_fail(lbs_client, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail(callback_type, LBS_CLIENT_ERROR_PARAMETER);

	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;
	g_return_val_if_fail(handle->is_started == FALSE, LBS_CLIENT_ERROR_STATUS);

	GVariant *reg = NULL;
	GVariant *param = NULL, *method = NULL;
	GError *error = NULL;
	GVariantBuilder *builder = NULL;
	GDBusProxy *proxy = NULL;
	gchar *signal_path = NULL;
	int ret = LBS_CLIENT_ERROR_NONE;

	signal_path = g_strdup_printf("%s/%s", SERVICE_PATH, "SAMSUNG");
	LBS_CLIENT_SECLOG("LBS signal subscribe Object Path [%s]", signal_path);

	if (callback) {
		handle->batch_cb = callback;
		handle->user_data = user_data;

		if (callback_type & LBS_CLIENT_BATCH_CB) {
			handle->batch_evt_id = g_dbus_connection_signal_subscribe(handle->conn,
			                                                          SERVICE_NAME,			/* Sender */
			                                                          "org.tizen.lbs.Batch",	/* Interface */
			                                                          "BatchChanged",			/* Member */
			                                                          signal_path,			/* Object */
			                                                          NULL,					/* arg0 */
			                                                          G_DBUS_SIGNAL_FLAGS_NONE,
			                                                          __signal_batch_callback, handle,
			                                                          NULL);

			if (handle->batch_evt_id) {
				LBS_CLIENT_LOGD("Listening batch info");
			} else {
				LBS_CLIENT_LOGD("Fail to listen batch info");
			}
		}
	}
	g_free(signal_path);

	/* Start Batch */
	LBS_CLIENT_LOGD("START: CMD-START_BATCH");
	builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(builder, "{sv}", "CMD", g_variant_new_string("START_BATCH"));
	g_variant_builder_add(builder, "{sv}", "BATCH_INTERVAL", g_variant_new_int32((gint32) batch_interval));
	g_variant_builder_add(builder, "{sv}", "BATCH_PERIOD", g_variant_new_int32((gint32) batch_period));
	param = g_variant_ref_sink(g_variant_new("(@a{sv})", g_variant_builder_end(builder)));
	method = g_variant_ref_sink(g_variant_new("(i)", handle->method));

	proxy = g_dbus_proxy_new_sync(handle->conn,  /* GDBusConnection */
	                              G_DBUS_PROXY_FLAGS_NONE, /*GDbusProxyFlags */
	                              NULL,
	                              SERVICE_NAME, SERVICE_PATH,
	                              "org.tizen.lbs.Manager",
	                              NULL,
	                              &error);

	if (error && error->message) {
		if (error->code == G_DBUS_ERROR_ACCESS_DENIED) {
			LBS_CLIENT_LOGE("Access denied. Msg[%s]", error->message);
			ret = LBS_CLIENT_ERROR_ACCESS_DENIED;
		} else {
			LBS_CLIENT_LOGE("Fail to new proxy ErrCode[%d], Msg[%s]", error->code, error->message);
			ret = LBS_CLIENT_ERROR_DBUS_CALL;
		}
		g_error_free(error);
		g_variant_unref(param);
		g_variant_unref(method);
		lbs_client_signal_unsubcribe(handle);
		return ret;
	}

	if (proxy) {
		reg = g_dbus_proxy_call_sync(proxy,
		                             "AddReference",
		                             method,
		                             G_DBUS_CALL_FLAGS_NONE,
		                             -1,
		                             NULL,
		                             &error);

		if (error && error->message) {
			if (error->code == G_DBUS_ERROR_ACCESS_DENIED) {
				LBS_CLIENT_LOGE("Access denied. Msg[%s]", error->message);
				ret = LBS_CLIENT_ERROR_ACCESS_DENIED;
			} else {
				LBS_CLIENT_LOGE("Fail to new proxy ErrCode[%d], Msg[%s]", error->code, error->message);
				ret = LBS_CLIENT_ERROR_DBUS_CALL;
			}
			g_error_free(error);
			g_variant_unref(param);
			g_variant_unref(method);
			lbs_client_signal_unsubcribe(handle);
			return ret;
		}
		if (reg) {
			g_variant_unref(reg);
			reg = NULL;
		}

		reg = g_dbus_proxy_call_sync(proxy,
		                             "SetOptions",
		                             param,
		                             G_DBUS_CALL_FLAGS_NO_AUTO_START,
		                             -1,
		                             NULL,
		                             &error);

		if (error && error->message) {
			if (error->code == G_DBUS_ERROR_ACCESS_DENIED) {
				LBS_CLIENT_LOGE("Access denied. Msg[%s]", error->message);
				ret = LBS_CLIENT_ERROR_ACCESS_DENIED;
			} else {
				LBS_CLIENT_LOGE("Fail to new proxy ErrCode[%d], Msg[%s]", error->code, error->message);
				ret = LBS_CLIENT_ERROR_DBUS_CALL;
			}
			g_error_free(error);
			g_variant_unref(param);
			g_variant_unref(method);

			lbs_client_signal_unsubcribe(handle);
			return ret;
		}
		if (reg) {
			g_variant_unref(reg);
			reg = NULL;
		}

		g_object_unref(proxy);
	}

	/* g_variant_builder_unref(builder); */
	g_variant_unref(method);
	g_variant_unref(param);

	lbs_client_privacy_signal_subcribe(handle);
	handle->is_started = TRUE;

	return LBS_CLIENT_ERROR_NONE;
}

EXPORT_API int
lbs_client_set_position_update_interval(lbs_client_dbus_h lbs_client, unsigned int interval)
{
	LBS_CLIENT_LOGD("lbs_client_set_position_update_interval");
	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;

	g_return_val_if_fail(handle, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail(handle->is_started == TRUE, LBS_CLIENT_ERROR_STATUS);

	GVariant *reg = NULL;
	GVariant *param = NULL;
	GVariant *method = NULL;
	GError *error = NULL;
	GVariantBuilder *builder = NULL;
	GDBusProxy *proxy = NULL;
	gchar *signal_path = NULL;

	signal_path = g_strdup_printf("%s/%s", SERVICE_PATH, "SAMSUNG");
	LBS_CLIENT_LOGD("LBS signal subscribe Object Path [%s]", signal_path);
	g_free(signal_path);

	LBS_CLIENT_LOGD("SET option INTERVAL_UPDATE:[%u], method:[%d]", interval, method);
	builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(builder, "{sv}", "CMD", g_variant_new_string("SET:OPT"));
	g_variant_builder_add(builder, "{sv}", "INTERVAL_UPDATE", g_variant_new_uint32(interval));
	g_variant_builder_add(builder, "{sv}", "METHOD", g_variant_new_int32(handle->method));
	param = g_variant_ref_sink(g_variant_new("(@a{sv})", g_variant_builder_end(builder)));
	method = g_variant_ref_sink(g_variant_new("(i)", handle->method));

	proxy = g_dbus_proxy_new_sync(handle->conn,  /* GDBusConnection */
	                              G_DBUS_PROXY_FLAGS_NONE, /*GDbusProxyFlags */
	                              NULL, SERVICE_NAME, SERVICE_PATH,
	                              "org.tizen.lbs.Manager",
	                              NULL,
	                              &error);
	if (proxy) {
		reg = g_dbus_proxy_call_sync(proxy,
		                             "SetOptions",
		                             param,
		                             G_DBUS_CALL_FLAGS_NO_AUTO_START,
		                             -1,
		                             NULL,
		                             &error);
		if (reg) {
			g_variant_unref(reg);
			reg = NULL;
		}

		g_object_unref(proxy);
	}
	/* g_variant_builder_unref(builder); */
	g_variant_unref(param);
	g_variant_unref(method);

	return LBS_CLIENT_ERROR_NONE;
}

EXPORT_API int
lbs_client_stop_batch(lbs_client_dbus_h lbs_client)
{
	LBS_CLIENT_LOGD("lbs_client_stop_batch");

	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;
	g_return_val_if_fail(handle, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail(handle->is_started == TRUE, LBS_CLIENT_ERROR_STATUS);

	int ret = LBS_CLIENT_ERROR_NONE;
	GVariant *param = NULL, *method = NULL, *reg = NULL;
	GError *error = NULL;
	GVariantBuilder *builder = NULL;

	lbs_client_privacy_signal_unsubcribe(handle);
	lbs_client_signal_unsubcribe(handle);

	/* Stop */
	LBS_CLIENT_LOGD("STOP: CMD-STOP_BATCH");
	builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(builder, "{sv}", "CMD", g_variant_new_string("STOP_BATCH"));

	LBS_CLIENT_LOGD("METHOD: %d", handle->method);
	g_variant_builder_add(builder, "{sv}", "METHOD", g_variant_new_int32(handle->method));
	param = g_variant_ref_sink(g_variant_new("(@a{sv})", g_variant_builder_end(builder)));
	method = g_variant_ref_sink(g_variant_new("(i)", handle->method));

	GDBusProxy *proxy = NULL;
	proxy = g_dbus_proxy_new_sync(handle->conn,  /* GDBusConnection */
	                              G_DBUS_PROXY_FLAGS_NONE, /*GDbusProxyFlags */
	                              NULL, SERVICE_NAME, SERVICE_PATH,
	                              "org.tizen.lbs.Manager",
	                              NULL,
	                              &error);

	if (error && error->message) {
		if (error->code == G_DBUS_ERROR_ACCESS_DENIED) {
			LBS_CLIENT_LOGI("Access denied. Msg[%s]", error->message);
			ret = LBS_CLIENT_ERROR_ACCESS_DENIED;
		} else {
			LBS_CLIENT_LOGI("Fail to new proxy ErrCode[%d], Msg[%s]", error->code, error->message);
			ret = LBS_CLIENT_ERROR_DBUS_CALL;
		}
		g_error_free(error);
		g_variant_unref(param);
		g_variant_unref(method);

		g_object_unref(proxy);
		return ret;
	}

	error = NULL;
	if (proxy) {
		reg = g_dbus_proxy_call_sync(proxy, "SetOptions", param, G_DBUS_CALL_FLAGS_NO_AUTO_START, -1, NULL, &error);
		if (error && error->message) {
			if (error->code == G_DBUS_ERROR_ACCESS_DENIED) {
				LBS_CLIENT_LOGI("Access denied. Msg[%s]", error->message);
				ret = LBS_CLIENT_ERROR_ACCESS_DENIED;
			} else {
				LBS_CLIENT_LOGI("Fail to proxy call. ErrCode[%d], Msg[%s]", error->code, error->message);
				ret = LBS_CLIENT_ERROR_DBUS_CALL;
			}
			g_error_free(error);
			g_variant_unref(param);
			g_variant_unref(method);

			g_object_unref(proxy);
			return ret;
		}

		if (reg) {
			g_variant_unref(reg);
			reg = NULL;
		}

		g_object_unref(proxy);
	}
	/* g_variant_builder_unref (builder); */
	g_variant_unref(param);
	g_variant_unref(method);

	handle->is_started = FALSE;

	return ret;
}

EXPORT_API int
lbs_client_start(lbs_client_dbus_h lbs_client, unsigned int interval, lbs_client_callback_e callback_type, lbs_client_cb callback, void *user_data)
{
	LBS_CLIENT_LOGD("lbs_client_start");

	g_return_val_if_fail(lbs_client, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail(callback_type, LBS_CLIENT_ERROR_PARAMETER);

	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;
	g_return_val_if_fail(handle->is_started == FALSE, LBS_CLIENT_ERROR_STATUS);

	GVariant *reg = NULL;
	GVariant *param = NULL, *method = NULL;
	GError *error = NULL;
	GVariantBuilder *builder = NULL;
	GDBusProxy *proxy = NULL;
	gchar *signal_path = NULL;
	gchar *app_id = NULL;
	int ret = LBS_CLIENT_ERROR_NONE;

	signal_path = g_strdup_printf("%s/%s", SERVICE_PATH, "SAMSUNG");
	LBS_CLIENT_SECLOG("LBS signal subscribe Object Path [%s]", signal_path);

	if (callback) {
		handle->user_cb = callback;
		handle->user_data = user_data;

		if (callback_type & LBS_CLIENT_LOCATION_CB) {
			handle->loc_evt_id = g_dbus_connection_signal_subscribe(
			                         handle->conn,
			                         SERVICE_NAME, /* Sender */
			                         "org.tizen.lbs.Position", /* Interface */
			                         "PositionChanged", /* Member */
			                         signal_path,
			                         NULL,
			                         G_DBUS_SIGNAL_FLAGS_NONE,
			                         __signal_callback, handle,
			                         NULL);

			if (handle->loc_evt_id) {
				LBS_CLIENT_LOGD("Listening Position info");
			} else {
				LBS_CLIENT_LOGD("Fail to listen Position info");
			}
		}

		if (callback_type & LBS_CLIENT_LOCATION_STATUS_CB) {
			handle->loc_status_evt_id = g_dbus_connection_signal_subscribe(handle->conn,
			                                                               SERVICE_NAME, /* Sender */
			                                                               "org.tizen.lbs.Manager", /* Interface */
			                                                               "StatusChanged", /* Member */
			                                                               SERVICE_PATH,
			                                                               NULL,
			                                                               G_DBUS_SIGNAL_FLAGS_NONE,
			                                                               __signal_callback, handle,
			                                                               NULL);

			if (handle->loc_status_evt_id) {
				LBS_CLIENT_LOGD("Listening location Status");
			} else {
				LBS_CLIENT_LOGD("Fail to listen location Status");
			}
		}

		if (callback_type & LBS_CLIENT_SATELLITE_CB) {
			handle->sat_evt_id = g_dbus_connection_signal_subscribe(handle->conn,
			                                                        SERVICE_NAME, /* Sender */
			                                                        "org.tizen.lbs.Satellite", /* Interface */
			                                                        "SatelliteChanged",	/* Member */
			                                                        signal_path,
			                                                        NULL,
			                                                        G_DBUS_SIGNAL_FLAGS_NONE,
			                                                        __signal_callback, handle,
			                                                        NULL);

			if (handle->sat_evt_id) {
				LBS_CLIENT_LOGD("Listening satellite info");
			} else {
				LBS_CLIENT_LOGD("Fail to listen satellite info");
			}
		}

		if (callback_type & LBS_CLIENT_NMEA_CB) {
			handle->nmea_evt_id = g_dbus_connection_signal_subscribe(handle->conn,
			                                                         SERVICE_NAME, /* Sender */
			                                                         "org.tizen.lbs.Nmea", /* Interface */
			                                                         "NmeaChanged",	/* Member */
			                                                         signal_path,
			                                                         NULL,
			                                                         G_DBUS_SIGNAL_FLAGS_NONE,
			                                                         __signal_callback, handle,
			                                                         NULL);

			if (handle->nmea_evt_id) {
				LBS_CLIENT_LOGD("Listening nmea info");
			} else {
				LBS_CLIENT_LOGD("Fail to listen nmea info");
			}
		}
	}
	g_free(signal_path);

	/* Start */
	LBS_CLIENT_LOGD("START: CMD-START");
	builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(builder, "{sv}", "CMD", g_variant_new_string("START"));

	g_variant_builder_add(builder, "{sv}", "METHOD", g_variant_new_int32(handle->method));
	g_variant_builder_add(builder, "{sv}", "INTERVAL", g_variant_new_uint32(interval));

	if (__lbs_get_appid(&app_id)) {
		LBS_CLIENT_LOGD("[%s] Request START", app_id);
		g_variant_builder_add(builder, "{sv}", "APP_ID", g_variant_new_string(app_id));
		if (app_id) {
			g_free(app_id);
		}
	}

	param = g_variant_ref_sink(g_variant_new("(@a{sv})", g_variant_builder_end(builder)));

	method = g_variant_ref_sink(g_variant_new("(i)", handle->method));

	proxy = g_dbus_proxy_new_sync(handle->conn,  /* GDBusConnection */
	                              G_DBUS_PROXY_FLAGS_NONE, /* GDbusProxyFlags */
	                              NULL, SERVICE_NAME, SERVICE_PATH,
	                              "org.tizen.lbs.Manager",
	                              NULL,
	                              &error);

	if (error && error->message) {
		if (error->code == G_DBUS_ERROR_ACCESS_DENIED) {
			LBS_CLIENT_LOGI("Access denied. Msg[%s]", error->message);
			ret = LBS_CLIENT_ERROR_ACCESS_DENIED;
		} else {
			LBS_CLIENT_LOGI("Fail to new proxy ErrCode[%d], Msg[%s]", error->code, error->message);
			ret = LBS_CLIENT_ERROR_DBUS_CALL;
		}
		g_error_free(error);
		g_variant_unref(param);
		g_variant_unref(method);
		lbs_client_signal_unsubcribe(handle);
		return ret;
	}

	if (proxy) {
		reg = g_dbus_proxy_call_sync(proxy,
		                             "AddReference",
		                             method,
		                             G_DBUS_CALL_FLAGS_NONE,
		                             -1,
		                             NULL,
		                             &error);

		if (error && error->message) {
			if (error->code == G_DBUS_ERROR_ACCESS_DENIED) {
				LBS_CLIENT_LOGI("Access denied. Msg[%s]", error->message);
				ret = LBS_CLIENT_ERROR_ACCESS_DENIED;
			} else {
				LBS_CLIENT_LOGI("Fail to new proxy ErrCode[%d], Msg[%s]", error->code, error->message);
				ret = LBS_CLIENT_ERROR_DBUS_CALL;
			}
			g_error_free(error);
			g_variant_unref(param);
			g_variant_unref(method);
			lbs_client_signal_unsubcribe(handle);
			return ret;
		}
		if (reg) {
			g_variant_unref(reg);
			reg = NULL;
		}

		reg = g_dbus_proxy_call_sync(proxy,
		                             "SetOptions",
		                             param,
		                             G_DBUS_CALL_FLAGS_NO_AUTO_START,
		                             -1,
		                             NULL,
		                             &error);
		if (error && error->message) {
			if (error->code == G_DBUS_ERROR_ACCESS_DENIED) {
				LBS_CLIENT_LOGI("Access denied. Msg[%s]", error->message);
				ret = LBS_CLIENT_ERROR_ACCESS_DENIED;
			} else {
				LBS_CLIENT_LOGI("Fail to new proxy ErrCode[%d], Msg[%s]", error->code, error->message);
				ret = LBS_CLIENT_ERROR_DBUS_CALL;
			}
			g_error_free(error);
			g_variant_unref(param);
			g_variant_unref(method);

			lbs_client_signal_unsubcribe(handle);
			return ret;
		}
		if (reg) {
			g_variant_unref(reg);
			reg = NULL;
		}

		g_object_unref(proxy);
	}

	/* g_variant_builder_unref (builder); */
	g_variant_unref(param);
	g_variant_unref(method);

	lbs_client_privacy_signal_subcribe(handle);
	handle->is_started = TRUE;

	return LBS_CLIENT_ERROR_NONE;
}

EXPORT_API int
lbs_client_stop(lbs_client_dbus_h lbs_client)
{
	LBS_CLIENT_LOGD("lbs_client_stop");

	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;
	g_return_val_if_fail(handle, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail(handle->is_started == TRUE, LBS_CLIENT_ERROR_STATUS);

	int ret = LBS_CLIENT_ERROR_NONE;
	GVariant *param = NULL, *method = NULL, *reg = NULL;
	GError *error = NULL;
	GVariantBuilder *builder = NULL;
	gchar *app_id = NULL;

	lbs_client_privacy_signal_unsubcribe(handle);
	lbs_client_signal_unsubcribe(handle);

	/* Stop */
	LBS_CLIENT_LOGD("STOP: CMD-STOP");
	builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(builder, "{sv}", "CMD", g_variant_new_string("STOP"));

	LBS_CLIENT_LOGD("METHOD: %d", handle->method);
	g_variant_builder_add(builder, "{sv}", "METHOD", g_variant_new_int32(handle->method));

	if (__lbs_get_appid(&app_id)) {
		LBS_CLIENT_LOGD("[%s] Request STOP", app_id);
		g_variant_builder_add(builder, "{sv}", "APP_ID", g_variant_new_string(app_id));
		if (app_id) {
			g_free(app_id);
		}
	}

	param = g_variant_ref_sink(g_variant_new("(@a{sv})", g_variant_builder_end(builder)));

	method = g_variant_ref_sink(g_variant_new("(i)", handle->method));

	GDBusProxy *proxy = NULL;
	proxy = g_dbus_proxy_new_sync(handle->conn,  /* GDBusConnection */
	                              G_DBUS_PROXY_FLAGS_NONE, /*GDbusProxyFlags */
	                              NULL, SERVICE_NAME, SERVICE_PATH,
	                              "org.tizen.lbs.Manager",
	                              NULL,
	                              &error);

	if (error && error->message) {
		if (error->code == G_DBUS_ERROR_ACCESS_DENIED) {
			LBS_CLIENT_LOGI("Access denied. Msg[%s]", error->message);
			ret = LBS_CLIENT_ERROR_ACCESS_DENIED;
		} else {
			LBS_CLIENT_LOGI("Fail to new proxy ErrCode[%d], Msg[%s]", error->code, error->message);
			ret = LBS_CLIENT_ERROR_DBUS_CALL;
		}
		g_error_free(error);
		g_variant_unref(param);
		g_variant_unref(method);

		g_object_unref(proxy);
		return ret;
	}

	error = NULL;
	if (proxy) {
		reg = g_dbus_proxy_call_sync(proxy, "SetOptions", param, G_DBUS_CALL_FLAGS_NO_AUTO_START, -1, NULL, &error);
		if (error && error->message) {
			if (error->code == G_DBUS_ERROR_ACCESS_DENIED) {
				LBS_CLIENT_LOGI("Access denied. Msg[%s]", error->message);
				ret = LBS_CLIENT_ERROR_ACCESS_DENIED;
			} else {
				LBS_CLIENT_LOGI("Fail to proxy call. ErrCode[%d], Msg[%s]", error->code, error->message);
				ret = LBS_CLIENT_ERROR_DBUS_CALL;
			}
			g_error_free(error);
			g_variant_unref(param);
			g_variant_unref(method);

			g_object_unref(proxy);
			return ret;
		}

		if (reg) {
			g_variant_unref(reg);
			reg = NULL;
		}

		reg = g_dbus_proxy_call_sync(proxy, "RemoveReference",
		                             method,
		                             G_DBUS_CALL_FLAGS_NO_AUTO_START,
		                             -1, NULL, &error);

		if (error && error->message) {
			if (error->code == G_DBUS_ERROR_ACCESS_DENIED) {
				LBS_CLIENT_LOGI("Access denied. Msg[%s]", error->message);
				ret = LBS_CLIENT_ERROR_ACCESS_DENIED;
			} else {
				LBS_CLIENT_LOGI("Fail to proxy call. ErrCode[%d], Msg[%s]", error->code, error->message);
				ret = LBS_CLIENT_ERROR_DBUS_CALL;
			}
			g_error_free(error);
			g_variant_unref(param);
			g_variant_unref(method);

			g_object_unref(proxy);
			return ret;
		}
		if (reg) {
			g_variant_unref(reg);
			reg = NULL;
		}

		g_object_unref(proxy);
	}
	/* g_variant_builder_unref (builder); */
	g_variant_unref(param);
	g_variant_unref(method);

	handle->is_started = FALSE;

	return ret;
}

EXPORT_API int
lbs_client_get_nmea(lbs_client_dbus_h lbs_client, int *timestamp, char **nmea)
{
	LBS_CLIENT_LOGD("ENTER >>>");
	g_return_val_if_fail(lbs_client, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail(timestamp, LBS_CLIENT_ERROR_PARAMETER);
	g_return_val_if_fail(nmea, LBS_CLIENT_ERROR_PARAMETER);

	GError *error = NULL;
	int ret = LBS_CLIENT_ERROR_NONE;

	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;
	*timestamp = 0;
	*nmea = NULL;

	LbsNmea *proxy = NULL;
	proxy = lbs_nmea_proxy_new_sync(handle->conn,
	                                G_DBUS_PROXY_FLAGS_NONE,
	                                SERVICE_NAME,
	                                SERVICE_PATH,
	                                NULL,
	                                &error);

	gint cur_timestamp = 0;
	gchar *cur_nmea_data = NULL;

	if (proxy) {
		ret = lbs_nmea_call_get_nmea_sync(proxy, &cur_timestamp, &cur_nmea_data, NULL, &error);
		if (error && error->message) {
			if (error->code == G_DBUS_ERROR_ACCESS_DENIED) {
				LBS_CLIENT_LOGE("Access denied. Msg[%s]", error->message);
				ret = LBS_CLIENT_ERROR_ACCESS_DENIED;
			} else {
				LBS_CLIENT_LOGE("Fail to new proxy ErrCode[%d], Msg[%s]", error->code, error->message);
				ret = LBS_CLIENT_ERROR_DBUS_CALL;
			}
			g_error_free(error);
			lbs_client_signal_unsubcribe(handle);
			return ret;
		}

		LBS_CLIENT_LOGD("Get NMEA: Timestamp[%d], NMEA[%s]", cur_timestamp, cur_nmea_data);
		*timestamp = cur_timestamp;
		*nmea = cur_nmea_data;

		g_object_unref(proxy);
	}

	return LBS_CLIENT_ERROR_NONE;
}

static int
_client_create_connection(lbs_client_dbus_s *client)
{
	LBS_CLIENT_LOGD("create connection");
	g_return_val_if_fail(client, LBS_CLIENT_ERROR_PARAMETER);
	GError *error = NULL;

	client->conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (!client->conn) {
		if (error && error->message) {
			LBS_CLIENT_LOGI("Fail to get GBus. ErrCode[%d], Msg[%s]", error->code, error->message);
			g_error_free(error);
			error = NULL;
		}
		LBS_CLIENT_LOGI("Fail to get addr of bus.");
		return LBS_CLIENT_ERROR_CONNECTION;
	}

	LBS_CLIENT_LOGD("client->conn: %p", client->conn);

	return LBS_CLIENT_ERROR_NONE;
}

static void _glib_log(const gchar *log_domain, GLogLevelFlags log_level,
                      const gchar *msg, gpointer user_data)
{
	LBS_CLIENT_LOGD("GLIB[%d] : %s", log_level, msg);
}

/* The reason why we seperate this from start is to support IPC for db operation between a server and a client. */
EXPORT_API int
lbs_client_create(lbs_client_method_e method, lbs_client_dbus_h *lbs_client)
{
	LBS_CLIENT_LOGD("lbs_client_create");
	g_return_val_if_fail(lbs_client, LBS_CLIENT_ERROR_PARAMETER);

	int ret = LBS_CLIENT_ERROR_NONE;

	lbs_client_dbus_s *client = g_new0(lbs_client_dbus_s, 1);
	g_return_val_if_fail(client, LBS_CLIENT_ERROR_MEMORY);

	g_log_set_default_handler(_glib_log, NULL);

	ret = _client_create_connection(client);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		g_free(client);
		return ret;
	}

	client->method = method;

	*lbs_client = (lbs_client_dbus_s *)client;

	return LBS_CLIENT_ERROR_NONE;
}

EXPORT_API int
lbs_client_destroy(lbs_client_dbus_h lbs_client)
{
	LBS_CLIENT_LOGD("lbs_client_destroy");
	g_return_val_if_fail(lbs_client, LBS_CLIENT_ERROR_PARAMETER);

	lbs_client_dbus_s *handle = (lbs_client_dbus_s *)lbs_client;
	g_return_val_if_fail(handle->is_started == FALSE, LBS_CLIENT_ERROR_STATUS);

	handle->user_cb = NULL;
	handle->batch_cb = NULL;
	handle->user_data = NULL;

	if (handle->conn) {
		g_object_unref(handle->conn);
		handle->conn = NULL;
	}

	g_free(handle);

	return LBS_CLIENT_ERROR_NONE;
}
