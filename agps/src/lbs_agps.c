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

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "lbs_agps.h"
#include "lbs_agps_priv.h"

#define SERVICE_NAME	"org.tizen.lbs.Providers.LbsServer"
#define SERVICE_PATH	"/org/tizen/lbs/Providers/LbsServer"
#define LBS_SERVER_METHOD_AGPS 2

typedef struct _lbs_agps_s {
	gchar *addr;
	GDBusConnection *conn;
} lbs_agps_s;

typedef void *lbs_agps_h;

static int
_create_connection(lbs_agps_s *agps)
{
	LBS_AGPS_LOGD("create connection");
	g_return_val_if_fail(agps, LBS_AGPS_ERROR_PARAMETER);
	GError *error = NULL;

	agps->addr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (!agps->addr) {
		LBS_AGPS_LOGD("Fail to get addr of bus.");
		return LBS_AGPS_ERROR_CONNECTION;
	}

	agps->conn = g_dbus_connection_new_for_address_sync(agps->addr,
	                                                    G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
	                                                    G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
	                                                    NULL, NULL, &error);
	if (!agps->conn) {
		LBS_AGPS_LOGD("Fail to get addr of bus.");
		g_free(agps->addr);
		return LBS_AGPS_ERROR_CONNECTION;
	}

	return LBS_AGPS_ERROR_NONE;
}


static int
lbs_agps_create(lbs_agps_h *lbs_agps)
{
	LBS_AGPS_LOGD("lbs_agps_create");

	int ret = LBS_AGPS_ERROR_NONE;

	lbs_agps_s *agps = g_new0(lbs_agps_s, 1);
	g_return_val_if_fail(agps, LBS_AGPS_ERROR_MEMORY);

	ret = _create_connection(agps);
	if (ret != LBS_AGPS_ERROR_NONE) {
		g_free(agps);
		return ret;
	}

	*lbs_agps = (lbs_agps_h *) agps;

	return LBS_AGPS_ERROR_NONE;
}

static int
lbs_agps_destroy(lbs_agps_h lbs_agps)
{
	LBS_AGPS_LOGD("lbs_agps_destroy");
	g_return_val_if_fail(lbs_agps, LBS_AGPS_ERROR_PARAMETER);

	lbs_agps_s *handle = (lbs_agps_s *) lbs_agps;

	int ret = LBS_AGPS_ERROR_NONE;
	GError *error = NULL;

	if (handle->conn) {
		ret = g_dbus_connection_close_sync(handle->conn, NULL, &error);
		if (ret != TRUE) {
			LBS_AGPS_LOGD("Fail to disconnection Dbus");
		}
		g_object_unref(handle->conn);
		handle->conn = NULL;
	}

	if (handle->addr) g_free(handle->addr);
	g_free(handle);

	return LBS_AGPS_ERROR_NONE;
}

EXPORT_API int
lbs_agps_sms(const char *msg_body, int msg_size)
{
	LBS_AGPS_LOGD("lbs_agps_sms");
	int ret = LBS_AGPS_ERROR_NONE;
	lbs_agps_h lbs_agps = NULL;
	GVariant *reg = NULL;
	GVariant *param = NULL;
	GVariant *method = NULL;
	GError *error = NULL;
	GVariantBuilder *builder = NULL;
	GDBusProxy *proxy = NULL;
	gchar *signal_path = NULL;

	ret = lbs_agps_create(&lbs_agps);
	if (ret != LBS_AGPS_ERROR_NONE || !lbs_agps) {
		LBS_AGPS_LOGE("Cannot get agps handle Error[%d]", ret);
		return ret;
	}
	lbs_agps_s *handle = (lbs_agps_s *) lbs_agps;

	signal_path = g_strdup_printf("%s/%s", SERVICE_PATH, "SAMSUNG");
	LBS_AGPS_LOGD("LBS signal subscribe Object Path [%s]", signal_path);
	g_free(signal_path);

	LBS_AGPS_LOGD("AGPS NI SMS START : SUPLNI");
	builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(builder, "{sv}", "CMD", g_variant_new_string("SUPLNI"));
	g_variant_builder_add(builder, "{sv}", "BODY", g_variant_new_from_data(G_VARIANT_TYPE_BYTESTRING, (gconstpointer) msg_body , (gsize) msg_size, TRUE, NULL, NULL));
	g_variant_builder_add(builder, "{sv}", "SIZE", g_variant_new_int32((gint32) msg_size));
	param = g_variant_ref_sink(g_variant_new("(@a{sv})", g_variant_builder_end(builder)));

	method = g_variant_ref_sink(g_variant_new("(i)", LBS_SERVER_METHOD_AGPS));

	proxy = g_dbus_proxy_new_sync(handle->conn,  /* GDBusConnection */
	                              G_DBUS_PROXY_FLAGS_NONE, /*GDbusProxyFlags */
	                              NULL,
	                              SERVICE_NAME, SERVICE_PATH,
	                              "org.tizen.lbs.Manager",
	                              NULL,
	                              &error);
	if (proxy) {
		reg = g_dbus_proxy_call_sync(proxy,
		                             "AddReference",
		                             method,
		                             G_DBUS_CALL_FLAGS_NONE,
		                             -1,
		                             NULL,
		                             &error);
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
		if (reg) {
			g_variant_unref(reg);
			reg = NULL;
		}

		reg = g_dbus_proxy_call_sync(proxy,
		                             "RemoveReference",
		                             method,
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
	g_variant_unref(method);
	g_variant_unref(param);

	lbs_agps_destroy(lbs_agps);

	return LBS_AGPS_ERROR_NONE;
}

EXPORT_API int
lbs_agps_wap_push(const char *push_header, const char *push_body, int push_body_size)
{
	LBS_AGPS_LOGD("lbs_agps_wap_push");
	int ret = LBS_AGPS_ERROR_NONE;
	lbs_agps_h lbs_agps = NULL;
	GVariant *reg = NULL;
	GVariant *param = NULL;
	GVariant *method = NULL;
	GError *error = NULL;
	GVariantBuilder *builder = NULL;
	GDBusProxy *proxy = NULL;
	gchar *signal_path = NULL;

	ret = lbs_agps_create(&lbs_agps);
	if (ret != LBS_AGPS_ERROR_NONE || !lbs_agps) {
		LBS_AGPS_LOGE("Cannot get agps handle Error[%d]", ret);
		return ret;
	}
	lbs_agps_s *handle = (lbs_agps_s *) lbs_agps;

	signal_path = g_strdup_printf("%s/%s", SERVICE_PATH, "SAMSUNG");
	LBS_AGPS_LOGD("LBS signal subscribe Object Path [%s]", signal_path);
	g_free(signal_path);

	LBS_AGPS_LOGD("AGPS NI WAP PUSH START : SUPLNI");
	builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(builder, "{sv}", "CMD", g_variant_new_string("SUPLNI"));
	g_variant_builder_add(builder, "{sv}", "HEADER", g_variant_new_string(push_header));
	g_variant_builder_add(builder, "{sv}", "BODY", g_variant_new_from_data(G_VARIANT_TYPE_BYTESTRING, (gconstpointer) push_body , (gsize) push_body_size, TRUE, NULL, NULL));
	g_variant_builder_add(builder, "{sv}", "SIZE", g_variant_new_int32((gint32) push_body_size));
	param = g_variant_ref_sink(g_variant_new("(@a{sv})", g_variant_builder_end(builder)));

	method = g_variant_ref_sink(g_variant_new("(i)", LBS_SERVER_METHOD_AGPS));


	proxy = g_dbus_proxy_new_sync(handle->conn,  /* GDBusConnection */
	                              G_DBUS_PROXY_FLAGS_NONE, /*GDbusProxyFlags */
	                              NULL,
	                              SERVICE_NAME, SERVICE_PATH,
	                              "org.tizen.lbs.Manager",
	                              NULL,
	                              &error);

	if (proxy) {
		reg = g_dbus_proxy_call_sync(proxy,
		                             "AddReference",
		                             method,
		                             G_DBUS_CALL_FLAGS_NONE,
		                             -1,
		                             NULL,
		                             &error);

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
		if (reg) {
			g_variant_unref(reg);
			reg = NULL;
		}

		reg = g_dbus_proxy_call_sync(proxy,
		                             "RemoveReference",
		                             method,
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

	lbs_agps_destroy(lbs_agps);

	return LBS_AGPS_ERROR_NONE;
}

EXPORT_API int
lbs_set_option(const char *option)
{
	LBS_AGPS_LOGD("lbs_set_option");
	int ret = LBS_AGPS_ERROR_NONE;
	lbs_agps_h lbs_agps = NULL;
	GVariant *reg = NULL;
	GVariant *param = NULL;
	GVariant *method = NULL;
	GError *error = NULL;
	GVariantBuilder *builder = NULL;
	GDBusProxy *proxy = NULL;
	gchar *signal_path = NULL;


	ret = lbs_agps_create(&lbs_agps);
	if (ret != LBS_AGPS_ERROR_NONE || !lbs_agps) {
		LBS_AGPS_LOGE("Cannot get agps handle Error[%d]", ret);
		return ret;
	}
	lbs_agps_s *handle = (lbs_agps_s *) lbs_agps;

	signal_path = g_strdup_printf("%s/%s", SERVICE_PATH, "SAMSUNG");
	LBS_AGPS_LOGD("LBS signal subscribe Object Path [%s]", signal_path);
	g_free(signal_path);

	LBS_AGPS_LOGD("SET option START : %s", option);
	builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(builder, "{sv}", "CMD", g_variant_new_string("SET:OPT"));
	g_variant_builder_add(builder, "{sv}", "OPTION", g_variant_new_string(option));
	param = g_variant_ref_sink(g_variant_new("(@a{sv})", g_variant_builder_end(builder)));

	method = g_variant_ref_sink(g_variant_new("(i)", LBS_SERVER_METHOD_AGPS));


	proxy = g_dbus_proxy_new_sync(handle->conn,  /* GDBusConnection */
	                              G_DBUS_PROXY_FLAGS_NONE, /*GDbusProxyFlags */
	                              NULL, SERVICE_NAME, SERVICE_PATH,
	                              "org.tizen.lbs.Manager",
	                              NULL,
	                              &error);
	if (proxy) {
		reg = g_dbus_proxy_call_sync(proxy,
		                             "AddReference",
		                             method,
		                             G_DBUS_CALL_FLAGS_NONE,
		                             -1,
		                             NULL,
		                             &error);
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
		if (reg) {
			g_variant_unref(reg);
			reg = NULL;
		}

		reg = g_dbus_proxy_call_sync(proxy,
		                             "RemoveReference",
		                             method,
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

	lbs_agps_destroy(lbs_agps);

	return LBS_AGPS_ERROR_NONE;
}
