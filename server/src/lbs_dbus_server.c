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

#include "generated-code.h"
#include "lbs_dbus_server.h"
#include "lbs_dbus_server_priv.h"


typedef struct _lbs_server_dbus_s {
	/* LBS server dbus info */
	gchar *service_name;
	gchar *prev_owner;
	gchar *service_path;
	gchar *name;
	gchar *description;
	GHashTable *connections;
	gint status;
	GDBusObjectManagerServer *manager;
	LbsObjectSkeleton *obj_skeleton;

	LbsDbusSetOptionsCB set_options_cb;
	LbsDbusShutdownCB shutdown_cb;
	gpointer userdata;  /* used for save GeoclueGpsManager */

	guint owner_changed_id;
	LbsManager *lbs_mgr;
	LbsPosition *lbs_pos;
	guint owner_id;
	guint get_providerinfo_h;
	guint get_status_h;
	guint set_option_h;
	guint add_reference_h;
	guint remove_reference_h;
} lbs_server_dbus_s;

static gboolean lbs_dbus_setup_position_interface(LbsObjectSkeleton *object, lbs_server_dbus_s *ctx)
{
	LBS_SERVER_LOGD("lbs_dbus_setup_position_interface");
	if (!object || !ctx) {
		return FALSE;
	}

	LbsPosition *position = NULL;
	position = lbs_position_skeleton_new();
	ctx->lbs_pos = position;
	lbs_object_skeleton_set_position(object, position);
	g_object_unref(position);

	LBS_SERVER_LOGD("position = %p", position);

	return TRUE;
}

static gboolean lbs_dbus_setup_satellite_interface(LbsObjectSkeleton *object, lbs_server_dbus_s *ctx)
{
	LBS_SERVER_LOGD("lbs_dbus_setup_satellite_interface");
	if (!object || !ctx) {
		return FALSE;
	}

	LbsSatellite *sat = NULL;
	sat = lbs_satellite_skeleton_new();
	lbs_object_skeleton_set_satellite(object, sat);
	g_object_unref(sat);

	LBS_SERVER_LOGD("sat = %p", sat);

	return TRUE;
}

static gboolean lbs_dbus_setup_nmea_interface(LbsObjectSkeleton *object, lbs_server_dbus_s *ctx)
{
	LBS_SERVER_LOGD("lbs_dbus_setup_nmea_interface");
	if (!object || !ctx) {
		return FALSE;
	}

	LbsNmea *nmea = NULL;
	nmea = lbs_nmea_skeleton_new();
	lbs_object_skeleton_set_nmea(object, nmea);
	g_object_unref(nmea);

	LBS_SERVER_LOGD("nmea = %p", nmea);

	return TRUE;
}



static gboolean
on_manager_getproviderinfo (LbsManager *mgr,
		GDBusMethodInvocation  *invocation,
		gpointer                user_data)
{
	LBS_SERVER_LOGD("on_manager_getproviderinfo");
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	if (ctx->name && ctx->description) {
		lbs_manager_complete_get_provider_info(mgr, invocation, ctx->name, ctx->description);
	} else {
		lbs_manager_complete_get_provider_info(mgr, invocation, NULL, NULL);
	}

	return TRUE;
}

static gboolean
on_manager_getstatus (LbsManager *mgr,
		GDBusMethodInvocation  *invocation,
		gpointer                user_data)
{
	LBS_SERVER_LOGD("on_manager_getstatus");
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	lbs_manager_complete_get_status(mgr, invocation, ctx->status);

	return TRUE;
}

static gboolean
on_manager_setoptions (LbsManager *mgr,
		GDBusMethodInvocation  *invocation,
		GVariant 	*options,
		gpointer	user_data)
{
	LBS_SERVER_LOGD("on_manager_setoptions");
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	if (ctx->set_options_cb) {
		ctx->set_options_cb(options, ctx->userdata);
		LBS_SERVER_LOGD("set_options_cb called");
	}

	lbs_manager_complete_set_options(mgr, invocation);

	return TRUE;
}

static gboolean
on_manager_addreference (LbsManager *mgr,
		GDBusMethodInvocation  *invocation,
		gpointer                user_data)
{
	LBS_SERVER_LOGD("on_manager_addreference");
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	const gchar *sender = NULL;
	gchar *sender_cp = NULL;
	int *pcount = NULL;

	/* Update the hash of open connections */
	sender = g_dbus_method_invocation_get_sender(invocation);
	sender_cp = g_strdup(sender);
	pcount = g_hash_table_lookup(ctx->connections, sender);
	if (!pcount) {
		pcount = g_malloc0(sizeof(int));
	}
	(*pcount)++;
	g_hash_table_insert(ctx->connections, (gpointer)sender_cp, pcount);
	LBS_SERVER_LOGD("sender [%s], pcount [%d] insert into hash table", sender_cp, *pcount);

	lbs_manager_complete_add_reference(mgr, invocation);

	return TRUE;
}

static gboolean
lbs_server_remove_client (lbs_server_dbus_s *ctx, const char *client)
{
	if (!ctx || !client) {
		return FALSE;
	}

	int *pcount = NULL;

	pcount = g_hash_table_lookup(ctx->connections, client);
	if (!pcount) {
		LBS_SERVER_LOGD("Client[%s] is already removed", client);
		return FALSE;
	}
	(*pcount)--;
	LBS_SERVER_LOGD("Client[%s] has reference count[%d]", client, *pcount);
	if (*pcount == 0) {
		LBS_SERVER_LOGD("Reference count is zero, Now remove client[%s] in hash table", client);
		g_hash_table_remove(ctx->connections, client);
	}
	if (g_hash_table_size(ctx->connections) == 0) {
		LBS_SERVER_LOGD("Hash table size is zero, Now we shutdown provider[%s]", ctx->name);
		if (ctx->shutdown_cb) {
			LBS_SERVER_LOGD("shutdown_cb to be called...");
			ctx->shutdown_cb(ctx->userdata);
			LBS_SERVER_LOGD("shutdown_cb called...");
		}
	}
	return TRUE;
}

static gboolean
on_manager_removereference (LbsManager *mgr,
		GDBusMethodInvocation  *invocation,
		gpointer                user_data)
{
	LBS_SERVER_LOGD("on_manager_removereference");
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	const gchar *sender = NULL;
	sender = g_dbus_method_invocation_get_sender(invocation);
	if (!lbs_server_remove_client(ctx, sender)) {
		LBS_SERVER_LOGD("Unreffed by client that has not been referenced");
	}

	lbs_manager_complete_remove_reference(mgr, invocation);

	return TRUE;
}

static gboolean
lbs_remove_client_by_force(const char *client, void *data)
{
	LBS_SERVER_LOGD("remove client by force");
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)data;
	if (lbs_server_remove_client (ctx, client)) {
		LBS_SERVER_LOGD("###### A clinet[%s] is abnormally shut down ########", client);
	}

	return TRUE;
}

static void
lbs_scan_sender(char *key, char *value, gpointer user_data)
{
	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)user_data;
	g_return_if_fail(handle);
	gchar *prev_owner = handle->prev_owner;
	g_return_if_fail(prev_owner);

	LBS_SERVER_LOGD("lbs_scan_sender >>  key[%s] : prev_owner[%s]\n", key, prev_owner);
	if (g_strcmp0(prev_owner, key) == 0) {
		LBS_SERVER_LOGD("disconnected sender name matched, remove client by force!");
		lbs_remove_client_by_force(prev_owner, handle);
	}
}

static void
on_name_owner_changed(GDBusConnection *connection,
                const gchar      *sender_name,
                const gchar      *object_path,
                const gchar      *interface_name,
                const gchar      *signal_name,
                GVariant         *parameters,	/* 1. service name 2. prev_owner 3. new_owner */
                gpointer         user_data)
{
	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)user_data;
	g_return_if_fail(handle);

	LBS_SERVER_LOGD("on_name_owner_changed: sender_name[%s], object_path[%s], interface_name[%s]",
		sender_name, object_path, interface_name);

	gchar *service_name = NULL, *prev_owner = NULL, *new_owner = NULL;
	g_variant_get (parameters, "(&s&s&s)", &service_name, &prev_owner, &new_owner);
	LBS_SERVER_LOGD("service_name %s, prev_owner %s, new_owner %s", service_name, prev_owner, new_owner);

	if (g_strcmp0 (object_path, "/org/freedesktop/DBus") != 0 ||
		g_strcmp0 (interface_name, "org.freedesktop.DBus") != 0 ||
		g_strcmp0 (sender_name, "org.freedesktop.DBus") != 0) {
		goto out;
	}

	/* if the prev_owner matches the sender name, then remote sender(client) is crashed */
	if (g_strcmp0 (new_owner, "") == 0 && (prev_owner != NULL && strlen(prev_owner) > 0) && handle->connections != NULL) {
		if (handle->prev_owner) {
			g_free(handle->prev_owner);
			handle->prev_owner = NULL;
		}
		handle->prev_owner = g_strdup(prev_owner);
		g_hash_table_foreach(handle->connections, (GHFunc)lbs_scan_sender, handle);
	}

out:
  	;
}

static void on_bus_acquired(GDBusConnection *conn, const gchar *name, gpointer user_data)
{
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return;
	}

	LbsManager *mgr = NULL;
	LbsObjectSkeleton *object = NULL;
	gchar *path = NULL;

	LBS_SERVER_LOGD("dbus registered");

	/* create object for each interfaces*/
	path = g_strdup_printf ("%s/%s", ctx->service_path, "SAMSUNG");
	object = lbs_object_skeleton_new(path);
	ctx->obj_skeleton = object;

	LBS_SERVER_LOGD("object path [%s], obj_skeleton [%p]", path, ctx->obj_skeleton);

	lbs_dbus_setup_position_interface(object, ctx);
	lbs_dbus_setup_satellite_interface(object, ctx);
	lbs_dbus_setup_nmea_interface(object, ctx);
	g_dbus_object_manager_server_export(ctx->manager, G_DBUS_OBJECT_SKELETON(object));

	/* Add interface to default object path */
	mgr = lbs_manager_skeleton_new();
	ctx->lbs_mgr = mgr;
	LBS_SERVER_LOGD("mgr = %p", mgr);

	ctx->get_providerinfo_h = g_signal_connect (mgr,
			"handle-get-provider-info",
			G_CALLBACK (on_manager_getproviderinfo),
			ctx); /* user_data */
	ctx->get_status_h = g_signal_connect (mgr,
			"handle-get-status",
			G_CALLBACK (on_manager_getstatus),
			ctx); /* user_data */
	ctx->set_option_h = g_signal_connect (mgr,
			"handle-set-options",
			G_CALLBACK (on_manager_setoptions),
			ctx); /* user_data */
	ctx->add_reference_h = g_signal_connect (mgr,
			"handle-add-reference",
			G_CALLBACK (on_manager_addreference),
			ctx); /* user_data */
	ctx->remove_reference_h = g_signal_connect (mgr,
			"handle-remove-reference",
			G_CALLBACK (on_manager_removereference),
			ctx); /* user_data */

	ctx->owner_changed_id = g_dbus_connection_signal_subscribe (conn,
				"org.freedesktop.DBus",
				"org.freedesktop.DBus",
				"NameOwnerChanged",
				"/org/freedesktop/DBus",
				NULL,
				G_DBUS_SIGNAL_FLAGS_NONE,
				on_name_owner_changed,
				ctx,
				NULL);

	g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(mgr), conn, ctx->service_path, NULL);

	g_dbus_object_manager_server_set_connection (ctx->manager, conn);

	LBS_SERVER_LOGD("done to acquire the dbus");
}

static void on_name_acquired(GDBusConnection *connection,
	const gchar     *name,
	gpointer         user_data)
{
	LBS_SERVER_LOGD("LBS Server: Acquired the name <%s> on the system bus", name);
}

static void on_name_lost(GDBusConnection *connection,
	const gchar     *name,
	gpointer         user_data)
{
	LBS_SERVER_LOGD("LBS Server: Lost the name <%s> on the system bus", name);
}

EXPORT_API int
lbs_server_emit_position_changed(lbs_server_dbus_h lbs_server,
				gint arg_fields,
				gint arg_timestamp,
				gdouble arg_latitude,
				gdouble arg_longitude,
				gdouble arg_altitude,
				gdouble arg_speed,
				gdouble arg_direction,
				gdouble arg_climb,
				GVariant *arg_accuracy)
{
	LBS_SERVER_LOGD("lbs_server_emit_position_changed");
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;
	g_return_val_if_fail(handle->obj_skeleton, LBS_SERVER_ERROR_PARAMETER);
	g_return_val_if_fail(handle->lbs_pos, LBS_SERVER_ERROR_PARAMETER);

	LBS_SERVER_LOGD("lbs_pos = %p", handle->lbs_pos);

	lbs_position_emit_position_changed(handle->lbs_pos,
				    arg_fields,
				    arg_timestamp,
				    arg_latitude,
				    arg_longitude,
				    arg_altitude,
				    arg_speed,
				    arg_direction,
				    arg_climb,
				    arg_accuracy);

	return LBS_SERVER_ERROR_NONE;

}

EXPORT_API int
lbs_server_emit_satellite_changed(lbs_server_dbus_h lbs_server,
			gint arg_timestamp,
			gint arg_satellite_used,
			gint arg_satellite_visible,
			GVariant *arg_used_prn,
			GVariant *arg_sat_info)
{
	LBS_SERVER_LOGD("lbs_server_emit_satellite_changed");
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;
	g_return_val_if_fail(handle->obj_skeleton, LBS_SERVER_ERROR_PARAMETER);

	LbsSatellite *sat = NULL;
	sat = lbs_object_peek_satellite(LBS_OBJECT(handle->obj_skeleton));
	LBS_SERVER_LOGD("sat = %p", sat);

	lbs_satellite_emit_satellite_changed (sat,
				arg_timestamp,
				arg_satellite_used,
				arg_satellite_visible,
				arg_used_prn,
				arg_sat_info);

	return LBS_SERVER_ERROR_NONE;
}

EXPORT_API int
lbs_server_emit_nmea_changed(lbs_server_dbus_h lbs_server,
			gint arg_timestamp,
			const gchar *arg_nmea_data)
{
	LBS_SERVER_LOGD("lbs_server_emit_nmea_changed");
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;
	g_return_val_if_fail(handle->obj_skeleton, LBS_SERVER_ERROR_PARAMETER);

	LbsNmea *nmea = NULL;
	nmea = lbs_object_peek_nmea(LBS_OBJECT(handle->obj_skeleton));
	LBS_SERVER_LOGD("nmea = %p", nmea);

	lbs_nmea_emit_nmea_changed(nmea, arg_timestamp, arg_nmea_data);

	return LBS_SERVER_ERROR_NONE;

}



EXPORT_API int
lbs_server_emit_status_changed(lbs_server_dbus_h lbs_server, gint status)
{
	LBS_SERVER_LOGD("lbs_server_emit_status_changed");
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;
	g_return_val_if_fail(handle->obj_skeleton, LBS_SERVER_ERROR_PARAMETER);
	g_return_val_if_fail(handle->lbs_mgr, LBS_SERVER_ERROR_PARAMETER);

	LBS_SERVER_LOGD("lbs_mgr = %p", handle->lbs_mgr);

	handle->status = status;
	lbs_manager_emit_status_changed(handle->lbs_mgr, status);

	return LBS_SERVER_ERROR_NONE;
}

EXPORT_API int
lbs_server_create(char *service_name,
			char *service_path,
			char *name,
			char *description,
			lbs_server_dbus_h *lbs_server,
			LbsDbusSetOptionsCB set_options_cb,
			LbsDbusShutdownCB shutdown_cb,
			gpointer userdata)
{
	LBS_SERVER_LOGD("lbs_server_create");
	g_return_val_if_fail (service_name, LBS_SERVER_ERROR_PARAMETER);
	g_return_val_if_fail (service_path, LBS_SERVER_ERROR_PARAMETER);
	g_return_val_if_fail (lbs_server, LBS_SERVER_ERROR_PARAMETER);

	int ret = LBS_SERVER_ERROR_NONE;

	lbs_server_dbus_s *server = g_new0(lbs_server_dbus_s, 1);
	g_return_val_if_fail (server, LBS_SERVER_ERROR_MEMORY);

	server->service_name = g_strdup(service_name);
	server->service_path = g_strdup(service_path);

	server->manager = g_dbus_object_manager_server_new(server->service_path);
	LBS_SERVER_LOGD("server->manager (%p)", server->manager);

	if (name) {
		server->name = g_strdup(name);
	}
	if (description) {
		server->description = g_strdup(description);
	}

	server->connections = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	server->userdata = userdata;
	server->set_options_cb = set_options_cb;
	server->shutdown_cb = shutdown_cb;

	server->owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM,
		service_name,
		G_BUS_NAME_OWNER_FLAGS_REPLACE,
		on_bus_acquired,
		on_name_acquired,
		on_name_lost,
		server,
		NULL);

	LBS_SERVER_LOGD("g_bus_own_name id=[%d]", server->owner_id);

	*lbs_server = (lbs_server_dbus_h *)server;

	return ret;
}

EXPORT_API int
lbs_server_destroy (lbs_server_dbus_h lbs_server)
{
	LBS_SERVER_LOGD("lbs_server_destroy");
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;

	int ret = LBS_SERVER_ERROR_NONE;

	g_bus_unown_name(handle->owner_id);

	if (handle->prev_owner) {
		g_free(handle->prev_owner);
		handle->prev_owner = NULL;
	}
	if (handle->service_path) {
		g_free(handle->service_path);
		handle->service_path = NULL;
	}
	if (handle->service_name) {
		g_free(handle->service_name);
		handle->service_name = NULL;
	}
	
	if (handle->get_providerinfo_h) {
		g_signal_handler_disconnect (handle->lbs_mgr, handle->get_providerinfo_h);
		handle->get_providerinfo_h = 0;
	}

	if (handle->get_status_h) {
		g_signal_handler_disconnect (handle->lbs_mgr, handle->get_status_h);
		handle->get_status_h = 0;
	}

	if (handle->set_option_h) {
		g_signal_handler_disconnect (handle->lbs_mgr, handle->set_option_h);
		handle->set_option_h = 0;
	}

	if (handle->add_reference_h) {
		g_signal_handler_disconnect (handle->lbs_mgr, handle->add_reference_h);
		handle->add_reference_h = 0;
	}

	if (handle->remove_reference_h) {
		g_signal_handler_disconnect (handle->lbs_mgr, handle->remove_reference_h);
		handle->remove_reference_h = 0;
	}

	if (handle->manager) {
		if(handle->owner_changed_id) {
			g_dbus_connection_signal_unsubscribe(g_dbus_object_manager_server_get_connection(handle->manager), handle->owner_changed_id);
			handle->owner_changed_id = 0;
		}
		g_object_unref(handle->manager);
		handle->manager = NULL;
	}

	g_hash_table_destroy(handle->connections);

	g_free(handle);

	return ret;
}

