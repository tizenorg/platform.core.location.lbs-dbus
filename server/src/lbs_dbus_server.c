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
	LbsDbusUpdateIntervalCB update_interval_cb;
	LbsDbusRequestChangeIntervalCB request_change_interval_cb;
	LbsDbusGetNmeaCB get_nmea_cb;
	gpointer userdata;	/* used for save GpsManager */

	guint owner_changed_id;
	guint owner_id;
	guint get_providerinfo_h;
	guint get_status_h;
	guint set_option_h;
	guint add_reference_h;
	guint remove_reference_h;
	guint get_nmea_h;

	/* for geofence */
	guint add_fence_h;
	guint remove_fence_h;
	guint pause_fence_h;
	guint resume_fence_h;
	guint start_geofence_h;
	guint stop_geofence_h;
	gint geofence_status;

	/* for H/W gps-geofence */
	guint add_hw_fence_h;
	guint delete_hw_fence_h;
	guint pause_hw_fence_h;
	guint resume_hw_fence_h;
	gint hw_geofence_status;
	GpsGeofenceAddFenceCB add_hw_fence_cb;
	GpsGeofenceDeleteFenceCB delete_hw_fence_cb;
	GpsGeofencePauseFenceCB pause_hw_fence_cb;
	GpsGeofenceResumeFenceCB resume_hw_fence_cb;
} lbs_server_dbus_s;

typedef enum {
    LBS_SERVER_METHOD_GPS = 0,
    LBS_SERVER_METHOD_NPS,
    LBS_SERVER_METHOD_AGPS,
    LBS_SERVER_METHOD_GEOFENCE,
    LBS_SERVER_METHOD_SIZE,
} lbs_server_method_e;

static gboolean lbs_dbus_setup_position_interface(LbsObjectSkeleton *object, lbs_server_dbus_s *ctx)
{
	LBS_SERVER_LOGD("lbs_dbus_setup_position_interface");
	if (!object || !ctx) {
		return FALSE;
	}

	LbsPosition *position = NULL;
	position = lbs_position_skeleton_new();
	lbs_object_skeleton_set_position(object, position);
	g_object_unref(position);

	return TRUE;
}

static gboolean lbs_dbus_setup_batch_interface(LbsObjectSkeleton *object, lbs_server_dbus_s *ctx)
{
	LBS_SERVER_LOGD("lbs_dbus_setup_batch_interface");
	if (!object || !ctx) {
		return FALSE;
	}

	LbsBatch *batch = NULL;
	batch = lbs_batch_skeleton_new();
	lbs_object_skeleton_set_batch(object, batch);
	g_object_unref(batch);

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

	return TRUE;
}

static gboolean lbs_dbus_setup_gps_geofence_interface(LbsObjectSkeleton *object, lbs_server_dbus_s *ctx)
{
	LBS_SERVER_LOGD("lbs_dbus_setup_gps_geofence_interface");
	if (!object || !ctx) {
		return FALSE;
	}

	LbsGpsGeofence *gps_geofence = NULL;
	gps_geofence = lbs_gps_geofence_skeleton_new();
	lbs_object_skeleton_set_gps_geofence(object, gps_geofence);
	g_object_unref(gps_geofence);

	return TRUE;
}


static gboolean
on_manager_getproviderinfo(LbsManager *mgr,
                           GDBusMethodInvocation	*invocation,
                           gpointer				user_data)
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
on_manager_getstatus(LbsManager *mgr,
                     GDBusMethodInvocation	*invocation,
                     gpointer				user_data)
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
on_nmea_getnmea(LbsNmea *nmea,
                GDBusMethodInvocation	*invocation,
                gpointer user_data)
{
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	gint timestamp = 0;
	gchar *nmea_data = NULL;

	if (ctx->get_nmea_cb) {
		ctx->get_nmea_cb(&timestamp, &nmea_data, ctx->userdata);
		LBS_SERVER_LOGD("timestmap: %d, nmea_data: %s", timestamp, nmea_data);
	}
	lbs_nmea_complete_get_nmea(nmea, invocation, timestamp, nmea_data);
	g_free(nmea_data);

	return TRUE;
}

static gboolean
on_manager_setoptions(LbsManager *mgr,
                      GDBusMethodInvocation *invocation,
                      GVariant	*options,
                      gpointer	user_data)
{
	LBS_SERVER_LOGD("ENTER >>>");
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	if (ctx->set_options_cb) {
		const gchar *sender = NULL;
		sender = g_dbus_method_invocation_get_sender(invocation);
		ctx->set_options_cb(options, sender, ctx->userdata);
		LBS_SERVER_LOGD("set_options_cb was called");
	}

	lbs_manager_complete_set_options(mgr, invocation);

	return TRUE;
}

static gboolean
on_manager_addreference(LbsManager *mgr,
                        GDBusMethodInvocation	*invocation,
                        int method,
                        gpointer				user_data)
{
	LBS_SERVER_LOGD("method: %d", method);
	if(method < 0 || method >= LBS_SERVER_METHOD_SIZE) return FALSE;

	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	const gchar *sender = NULL;
	gchar *sender_cp = NULL;
	int count = 0;

	/* Update the hash of open connections */
	sender = g_dbus_method_invocation_get_sender(invocation);
	sender_cp = g_strdup(sender);

	int *count_arr = (int *) g_hash_table_lookup(ctx->connections, sender_cp);
	if (!count_arr) {
		LBS_SERVER_LOGD("first add for sender %s ", sender_cp);
		count_arr = (int *)g_malloc0(LBS_SERVER_METHOD_SIZE * sizeof(int));
		g_return_val_if_fail(count_arr, FALSE);

		g_hash_table_insert(ctx->connections, (gpointer)sender_cp, (gpointer)count_arr);
	}

	count = count_arr[method];

	LBS_SERVER_LOGD("sender: [%s] method:%d count:%d table:%p", sender_cp, method, count, count_arr);
	count++;

	if (count <= 0) {
		count = 1;
		LBS_SERVER_LOGE("Client reference count set to 1 for sender [%s] of method [%d]", sender_cp, method);
	}

	LBS_SERVER_LOGD("sender [%s], method[%d], count [%d] is inserted in hash table", sender_cp, method, count);

	count_arr[method] = count;

	lbs_manager_complete_add_reference(mgr, invocation);

	return TRUE;
}

static gboolean lbs_find_method(gpointer key, gpointer value, gpointer user_data)
{
	int *ip = (int *) user_data;
	int *arr = (int *) value;
	int method = *ip;

	LBS_SERVER_LOGD("[%s] lbs_find_method method:%d, count:%d", (char *)key, method, arr[method]);

	return (arr[method] > 0) ? TRUE : FALSE;
}

static gboolean
lbs_server_remove_client(lbs_server_dbus_s *ctx, const char *client, int method)
{
	if (!ctx || !client) {
		return FALSE;
	}

	int count = 0;
	int *count_arr = (int *) g_hash_table_lookup(ctx->connections, client);

	if (!count_arr) {
		LBS_SERVER_LOGD("Client[%s] Method[%d] is already removed", client, method);
		return FALSE;
	}

	count = count_arr[method];
	LBS_SERVER_LOGD("lbs_server_remove_client method:%d count:%d", method, count);

	if (count == 0) {
		LBS_SERVER_LOGD("Client[%s] Method[%d] is already removed", client, method);
		return FALSE;
	}

	count--;
	count_arr[method] = count;

	if (count > 0) {
		LBS_SERVER_LOGD("Client[%s] of method[%d] has reference count[%d]", client, method, count);
	} else if (count == 0) {
		LBS_SERVER_LOGD("Remove [%s : %d] in hash table, ref count is 0", client, method);

		int i = 0, count_each = 0;
		for (i = 0; i < LBS_SERVER_METHOD_SIZE; i++) {
			count_each = count_arr[i];
			if (count_each != 0) {
				LBS_SERVER_LOGD("[%s] method[%d]'s count is not zero - count: %d", client, i, count_each);
				return FALSE;
			}
		}

		if (!g_hash_table_remove(ctx->connections, client)) {
			LBS_SERVER_LOGE("g_hash_table_remove is Fail");
		}
	}

	int index = 0;
	gboolean *shutdown_arr = (gboolean *) g_malloc0_n(LBS_SERVER_METHOD_SIZE, sizeof(gboolean));
	g_return_val_if_fail(shutdown_arr, FALSE);

	if (g_hash_table_size(ctx->connections) == 0) {
		LBS_SERVER_SECLOG("Hash table size is zero, Now shutdown provider[%s]", ctx->name);

		for (; index < LBS_SERVER_METHOD_SIZE; index++)	shutdown_arr[index] = TRUE;
	} else {
		LBS_SERVER_SECLOG("Hash table size is not zero");

		for (; index < LBS_SERVER_METHOD_SIZE ; index++) {
			if (g_hash_table_find(ctx->connections, (GHRFunc)lbs_find_method, &index) == NULL) {
				shutdown_arr[index] = TRUE;
				continue;
			}
		}
	}

	if (ctx->shutdown_cb) {
		ctx->shutdown_cb(ctx->userdata, shutdown_arr);
		LBS_SERVER_LOGD("shutdown_cb called.. gps:%d, nps:%d",
		                shutdown_arr[LBS_SERVER_METHOD_GPS], shutdown_arr[LBS_SERVER_METHOD_NPS]);
	}

	g_free(shutdown_arr);
	return TRUE;
}

static gboolean on_manager_removereference(LbsManager *mgr,
                                           GDBusMethodInvocation	*invocation,
                                           int						method,
                                           gpointer				user_data)
{
	LBS_SERVER_LOGD("method: %d", method);
	if(method < 0 || method >= LBS_SERVER_METHOD_SIZE) return FALSE;

	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	const gchar *sender = NULL;
	sender = g_dbus_method_invocation_get_sender(invocation);
	if (!lbs_server_remove_client(ctx, sender, method)) {
		LBS_SERVER_LOGD("Unreffed by client that has not been referenced");
	}

	lbs_manager_complete_remove_reference(mgr, invocation);

	return TRUE;
}

/*
 * For H/W gps-geofence methods
 */
static gboolean
on_gps_geofence_addfence(LbsGpsGeofence *gps_geofence,
                         GDBusMethodInvocation *invocation,
                         gint fence_id,
                         gdouble latitude,
                         gdouble longitude,
                         gint radius,
                         gint last_state,
                         gint monitor_states,
                         gint notification_responsiveness,
                         gint unknown_timer,
                         gpointer	user_data)
{
	LBS_SERVER_LOGD("on_gps_geofence_addfence");

	/* call gps-manager's callback, add_hw_fence_cb */
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	if (ctx->add_hw_fence_cb) {
		ctx->add_hw_fence_cb(fence_id, latitude, longitude, radius, last_state, monitor_states,
		                     notification_responsiveness, unknown_timer, ctx->userdata);
		LBS_SERVER_LOGD("add_hw_fence_cb called");
	}
	lbs_gps_geofence_complete_add_fence(gps_geofence, invocation);
	return TRUE;
}

static gboolean
on_gps_geofence_deletefence(LbsGpsGeofence *gps_geofence,
                            GDBusMethodInvocation	*invocation,
                            gint					fence_id,
                            gpointer				user_data)
{
	LBS_SERVER_LOGD("on_gps_geofence_deletefence");

	/* call gps-manager's callback, delete_hw_fence_cb */
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	if (ctx->delete_hw_fence_cb) {
		ctx->delete_hw_fence_cb(fence_id, ctx->userdata);
		LBS_SERVER_LOGD("delete_hw_fence_cb called");
	}
	lbs_gps_geofence_complete_delete_fence(gps_geofence, invocation);
	return TRUE;
}

static gboolean
on_gps_geofence_pausefence(LbsGpsGeofence *gps_geofence,
                           GDBusMethodInvocation	*invocation,
                           gint fence_id,
                           gpointer	user_data)
{
	LBS_SERVER_LOGD("on_gps_geofence_pausefence");

	/* call gps-manager's callback, pause_hw_fence_cb */
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	if (ctx->pause_hw_fence_cb) {
		ctx->pause_hw_fence_cb(fence_id, ctx->userdata);
		LBS_SERVER_LOGD("pause_hw_fence_cb called");
	}

	lbs_gps_geofence_complete_pause_fence(gps_geofence, invocation);
	return TRUE;
}

static gboolean
on_gps_geofence_resumefence(LbsGpsGeofence *gps_geofence,
                            GDBusMethodInvocation	*invocation,
                            gint fence_id,
                            gint monitor_states,
                            gpointer	user_data)
{
	LBS_SERVER_LOGD("on_gps_geofence_resumefence");

	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)user_data;
	if (!ctx) {
		return FALSE;
	}

	/* call gps-manager's callback, resume_hw_fence_cb */
	if (ctx->resume_hw_fence_cb) {
		ctx->resume_hw_fence_cb(fence_id, monitor_states, ctx->userdata);
		LBS_SERVER_LOGD("resume_hw_fence_cb called");
	}

	lbs_gps_geofence_complete_resume_fence(gps_geofence, invocation);
	return TRUE;
}

static gboolean
lbs_remove_client_by_force(const char *client, void *data)
{
	LBS_SERVER_LOGD("remove client by force for client [%s]", client);
	lbs_server_dbus_s *ctx = (lbs_server_dbus_s *)data;

	int *count_arr = (int *) g_hash_table_lookup(ctx->connections, client);

	if (!count_arr) {
		LBS_SERVER_LOGD("Client[%s] is already removed", client);
		return FALSE;
	} else {
		LBS_SERVER_LOGD("[Client: %s]. Remove all clients in hash table", client);
		if (!g_hash_table_remove(ctx->connections, client)) {
			LBS_SERVER_LOGE("g_hash_table_remove is Fail");
		}
	}

	int index = 0;
	gboolean *shutdown_arr = (gboolean *) g_malloc0_n(LBS_SERVER_METHOD_SIZE, sizeof(gboolean));
	g_return_val_if_fail(shutdown_arr, FALSE);

	if (g_hash_table_size(ctx->connections) == 0) {
		LBS_SERVER_SECLOG("Hash table size is zero, Now shutdown provider[%s]", ctx->name);

		for (; index < LBS_SERVER_METHOD_SIZE; index++)	shutdown_arr[index] = TRUE;
	} else {
		LBS_SERVER_SECLOG("Hash table size is not zero");

		for (; index < LBS_SERVER_METHOD_SIZE ; index++) {
			if (g_hash_table_find(ctx->connections, (GHRFunc)lbs_find_method, &index) == NULL) {
				shutdown_arr[index] = TRUE;
				continue;
			}
		}
	}

	if (ctx->shutdown_cb) {
		ctx->shutdown_cb(ctx->userdata, shutdown_arr);
		LBS_SERVER_LOGD("shutdown_cb called.. gps:%d, nps:%d",
		                shutdown_arr[LBS_SERVER_METHOD_GPS], shutdown_arr[LBS_SERVER_METHOD_NPS]);
	}

	if (ctx->update_interval_cb) {
		gboolean is_needed_change_interval = FALSE;
		for (index = 0; index < LBS_SERVER_METHOD_SIZE ; index++) {
			is_needed_change_interval = ctx->update_interval_cb(LBS_SERVER_INTERVAL_REMOVE, client, index, 0, ctx->userdata);
			if (is_needed_change_interval) {
				is_needed_change_interval = FALSE;
				if (ctx->request_change_interval_cb)
					ctx->request_change_interval_cb(index, ctx->userdata);
			}
		}

	}

	LBS_SERVER_LOGD("###### A client[%s] is abnormally shut down ########", client);

	g_free(shutdown_arr);
	return TRUE;
}

static void
lbs_scan_sender(char *key, char *value, gpointer user_data)
{
	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)user_data;
	g_return_if_fail(handle);
	gchar *prev_owner = handle->prev_owner;
	g_return_if_fail(prev_owner);

	if (g_strcmp0(prev_owner, key) == 0) {
		LBS_SERVER_LOGD("disconnected sender name matched, remove client by force!");
		lbs_remove_client_by_force(prev_owner, handle);
	}
}

static void
on_name_owner_changed(GDBusConnection *connection,
                      const gchar		*sender_name,
                      const gchar		*object_path,
                      const gchar		*interface_name,
                      const gchar		*signal_name,
                      GVariant		*parameters,	/* 1. service name 2. prev_owner 3. new_owner */
                      gpointer		user_data)
{
	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)user_data;
	g_return_if_fail(handle);

	gchar *service_name = NULL, *prev_owner = NULL, *new_owner = NULL;
	g_variant_get(parameters, "(&s&s&s)", &service_name, &prev_owner, &new_owner);

	if (g_strcmp0(object_path, "/org/freedesktop/DBus") != 0 ||
	    g_strcmp0(interface_name, "org.freedesktop.DBus") != 0 ||
	    g_strcmp0(sender_name, "org.freedesktop.DBus") != 0) {
		goto out;
	}

	/* if the prev_owner matches the sender name, then remote sender(client) is crashed */
	if (g_strcmp0(new_owner, "") == 0 && (prev_owner != NULL && strlen(prev_owner) > 0)
	    && handle->connections != NULL) {
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
	path = g_strdup_printf("%s/%s", ctx->service_path, "SAMSUNG");
	if (path == NULL) {
		LBS_SERVER_LOGE("path is NULL");
		path = NULL;
	}

	object = lbs_object_skeleton_new(path);
	g_free(path);
	if (object == NULL) {
		LBS_SERVER_LOGE("Can't create object. path: %s", path);
		return;
	}

	ctx->obj_skeleton = object;
	lbs_dbus_setup_position_interface(object, ctx);
	lbs_dbus_setup_batch_interface(object, ctx);
	lbs_dbus_setup_satellite_interface(object, ctx);
	lbs_dbus_setup_nmea_interface(object, ctx);

	/* add H/W gps-geofence interface */
	lbs_dbus_setup_gps_geofence_interface(object, ctx);
	g_dbus_object_manager_server_export(ctx->manager, G_DBUS_OBJECT_SKELETON(object));

	/* Add interface to default object path */
	mgr = lbs_manager_skeleton_new();

	ctx->get_providerinfo_h = g_signal_connect(mgr,
	                                           "handle-get-provider-info",
	                                           G_CALLBACK(on_manager_getproviderinfo),
	                                           ctx); /* user_data */
	ctx->get_status_h = g_signal_connect(mgr,
	                                     "handle-get-status",
	                                     G_CALLBACK(on_manager_getstatus),
	                                     ctx); /* user_data */
	if (ctx->set_options_cb != NULL) {
		ctx->set_option_h = g_signal_connect(mgr,
		                                     "handle-set-options",
		                                     G_CALLBACK(on_manager_setoptions),
		                                     ctx); /* user_data */
	}
	ctx->add_reference_h = g_signal_connect(mgr,
	                                        "handle-add-reference",
	                                        G_CALLBACK(on_manager_addreference),
	                                        ctx); /* user_data */
	if (ctx->shutdown_cb) {
		ctx->remove_reference_h = g_signal_connect(mgr,
		                                           "handle-remove-reference",
		                                           G_CALLBACK(on_manager_removereference),
		                                           ctx); /* user_data */
	}

	/* Add interface for nmea method*/
	LbsNmea *nmea = NULL;
	nmea = lbs_nmea_skeleton_new();
	ctx->get_nmea_h = g_signal_connect(nmea,
	                                   "handle-get-nmea",
	                                   G_CALLBACK(on_nmea_getnmea),
	                                   ctx); /* user_data */

	/* register callback for each methods for H/W gps-geofence */
	LbsGpsGeofence *gps_geofence = NULL;
	if (ctx->obj_skeleton) {
		gps_geofence = lbs_object_get_gps_geofence(LBS_OBJECT(ctx->obj_skeleton));
		if (gps_geofence) {
			if (ctx->add_hw_fence_cb) {
				ctx->add_hw_fence_h = g_signal_connect(gps_geofence,
				                                       "handle-add-fence",
				                                       G_CALLBACK(on_gps_geofence_addfence),
				                                       ctx); /* user_data */
			}
			if (ctx->delete_hw_fence_cb) {
				ctx->delete_hw_fence_h = g_signal_connect(gps_geofence,
				                                          "handle-delete-fence",
				                                          G_CALLBACK(on_gps_geofence_deletefence),
				                                          ctx); /* user_data */
			}
			if (ctx->pause_hw_fence_cb) {
				ctx->pause_hw_fence_h = g_signal_connect(gps_geofence,
				                                         "handle-pause-fence",
				                                         G_CALLBACK(on_gps_geofence_pausefence),
				                                         ctx); /* user_data */
			}
			if (ctx->resume_hw_fence_cb) {
				ctx->resume_hw_fence_h = g_signal_connect(gps_geofence,
				                                          "handle-resume-fence",
				                                          G_CALLBACK(on_gps_geofence_resumefence),
				                                          ctx); /* user_data */
			}
			g_object_unref(gps_geofence);
		}
	}

	ctx->owner_changed_id = g_dbus_connection_signal_subscribe(conn,
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
	g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(nmea), conn, ctx->service_path, NULL);
	/*	g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(nmea), conn, path, NULL); */

	g_dbus_object_manager_server_set_connection(ctx->manager, conn);

	LBS_SERVER_LOGD("done to acquire the dbus");
}

static void on_name_acquired(GDBusConnection *connection,
                             const gchar		*name,
                             gpointer		user_data)
{
	LBS_SERVER_SECLOG("LBS Server: Acquired the name <%s> on the system bus", name);
}

static void on_name_lost(GDBusConnection *connection,
                         const gchar		*name,
                         gpointer		user_data)
{
	LBS_SERVER_SECLOG("LBS Server: Lost the name <%s> on the system bus", name);
}

EXPORT_API int
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
                                 GVariant *arg_accuracy)
{
	LBS_SERVER_LOGD("method:%d", arg_method);
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);
	g_return_val_if_fail(arg_accuracy, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;
	g_return_val_if_fail(handle->obj_skeleton, LBS_SERVER_ERROR_PARAMETER);

	LbsPosition *lbs_pos = NULL;
	lbs_pos = lbs_object_get_position(LBS_OBJECT(handle->obj_skeleton));
	g_return_val_if_fail(lbs_pos, LBS_SERVER_ERROR_PARAMETER);

	lbs_position_emit_position_changed(lbs_pos,
	                                   arg_method,
	                                   arg_fields,
	                                   arg_timestamp,
	                                   arg_latitude,
	                                   arg_longitude,
	                                   arg_altitude,
	                                   arg_speed,
	                                   arg_direction,
	                                   arg_climb,
	                                   arg_accuracy);

	g_object_unref(lbs_pos);

	return LBS_SERVER_ERROR_NONE;

}

EXPORT_API int
lbs_server_emit_batch_changed(lbs_server_dbus_h lbs_server,
                              gint arg_num_of_location)
{
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;
	g_return_val_if_fail(handle->obj_skeleton, LBS_SERVER_ERROR_PARAMETER);

	LbsBatch *lbs_batch = NULL;
	lbs_batch = lbs_object_get_batch(LBS_OBJECT(handle->obj_skeleton));
	g_return_val_if_fail(lbs_batch, LBS_SERVER_ERROR_PARAMETER);

	lbs_batch_emit_batch_changed(lbs_batch,
	                             arg_num_of_location);

	g_object_unref(lbs_batch);

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
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);
	g_return_val_if_fail(arg_used_prn, LBS_SERVER_ERROR_PARAMETER);
	g_return_val_if_fail(arg_sat_info, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;
	g_return_val_if_fail(handle->obj_skeleton, LBS_SERVER_ERROR_PARAMETER);

	LbsSatellite *sat = NULL;
	sat = lbs_object_get_satellite(LBS_OBJECT(handle->obj_skeleton));
	g_return_val_if_fail(sat, LBS_SERVER_ERROR_PARAMETER);

	lbs_satellite_emit_satellite_changed(sat,
	                                     arg_timestamp,
	                                     arg_satellite_used,
	                                     arg_satellite_visible,
	                                     arg_used_prn,
	                                     arg_sat_info);
	g_object_unref(sat);

	return LBS_SERVER_ERROR_NONE;
}

EXPORT_API int
lbs_server_emit_nmea_changed(lbs_server_dbus_h lbs_server,
                             gint arg_timestamp,
                             const gchar *arg_nmea_data)
{
	LBS_SERVER_LOGW("timestamp: %d, nmea_data: %s", arg_timestamp, arg_nmea_data);

	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);
	g_return_val_if_fail(arg_nmea_data, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;
	g_return_val_if_fail(handle->obj_skeleton, LBS_SERVER_ERROR_PARAMETER);

	LbsNmea *nmea = NULL;
	nmea = lbs_object_get_nmea(LBS_OBJECT(handle->obj_skeleton));
	g_return_val_if_fail(nmea, LBS_SERVER_ERROR_PARAMETER);

	lbs_nmea_emit_nmea_changed(nmea, arg_timestamp, arg_nmea_data);
	g_object_unref(nmea);

	return LBS_SERVER_ERROR_NONE;
}

EXPORT_API int
lbs_server_emit_status_changed(lbs_server_dbus_h lbs_server, int method, gint status)
{
	LBS_SERVER_LOGD("method: %d, status: %d", method, status);
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;
	g_return_val_if_fail(handle->obj_skeleton, LBS_SERVER_ERROR_PARAMETER);

	LbsManager *lbs_mgr = NULL;
	lbs_mgr = lbs_object_get_manager(LBS_OBJECT(handle->obj_skeleton));
	g_return_val_if_fail(lbs_mgr, LBS_SERVER_ERROR_PARAMETER);

	handle->status = status;
	lbs_manager_emit_status_changed(lbs_mgr, method, status);
	g_object_unref(lbs_mgr);

	return LBS_SERVER_ERROR_NONE;
}

/* gps-manager -> geofence-manager : enable/disable */
EXPORT_API int
lbs_server_emit_gps_geofence_status_changed(lbs_server_dbus_h lbs_server, gint status)
{
	LBS_SERVER_LOGD("ENTER >>>");
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;
	g_return_val_if_fail(handle->obj_skeleton, LBS_SERVER_ERROR_PARAMETER);

	LbsGpsGeofence *gps_geofence = NULL;
	gps_geofence = lbs_object_get_gps_geofence(LBS_OBJECT(handle->obj_skeleton));
	g_return_val_if_fail(gps_geofence, LBS_SERVER_ERROR_PARAMETER);

	handle->hw_geofence_status = status;
	lbs_gps_geofence_emit_status_changed(gps_geofence, status);
	g_object_unref(gps_geofence);

	return LBS_SERVER_ERROR_NONE;
}

/* gps-manager -> geofence-manger: fence in/out */
EXPORT_API int
lbs_server_emit_gps_geofence_changed(lbs_server_dbus_h lbs_server, gint fence_id, gint transition, gdouble latitude, gdouble longitude, gdouble altitude, gdouble speed, gdouble bearing, gdouble hor_accuracy)
{
	LBS_SERVER_LOGD("ENTER >>>");
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;
	g_return_val_if_fail(handle->obj_skeleton, LBS_SERVER_ERROR_PARAMETER);

	LbsGpsGeofence *gps_geofence = NULL;
	gps_geofence = lbs_object_get_gps_geofence(LBS_OBJECT(handle->obj_skeleton));
	g_return_val_if_fail(gps_geofence, LBS_SERVER_ERROR_PARAMETER);

	lbs_gps_geofence_emit_geofence_changed(gps_geofence, fence_id, transition, latitude, longitude, altitude, speed, bearing, hor_accuracy);
	g_object_unref(gps_geofence);

	return LBS_SERVER_ERROR_NONE;
}

static void _glib_log(const gchar *log_domain, GLogLevelFlags log_level,
                      const gchar *msg, gpointer user_data)
{
	LBS_SERVER_LOGD("GLIB[%d]: %s", log_level, msg);
}

EXPORT_API int
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
                  gpointer userdata)

{
	LBS_SERVER_LOGD("ENTER >>>");
	g_return_val_if_fail(service_name, LBS_SERVER_ERROR_PARAMETER);
	g_return_val_if_fail(service_path, LBS_SERVER_ERROR_PARAMETER);
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);

	int ret = LBS_SERVER_ERROR_NONE;

	lbs_server_dbus_s *server = g_new0(lbs_server_dbus_s, 1);
	g_return_val_if_fail(server, LBS_SERVER_ERROR_MEMORY);

	g_log_set_default_handler(_glib_log, NULL);

	server->service_name = g_strdup(service_name);
	server->service_path = g_strdup(service_path);

	server->manager = g_dbus_object_manager_server_new(server->service_path);

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
	server->update_interval_cb = update_interval_cb;
	server->request_change_interval_cb = request_change_interval_cb;
	server->get_nmea_cb = get_nmea_cb;

	/* add H/W gps-gefence callbacks */
	server->add_hw_fence_cb = add_hw_fence_cb;
	server->delete_hw_fence_cb = delete_hw_fence_cb;
	server->pause_hw_fence_cb = pause_hw_fence_cb;
	server->resume_hw_fence_cb = resume_hw_fence_cb;

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
lbs_server_destroy(lbs_server_dbus_h lbs_server)
{
	LBS_SERVER_LOGD("ENTER >>>");
	g_return_val_if_fail(lbs_server, LBS_SERVER_ERROR_PARAMETER);

	lbs_server_dbus_s *handle = (lbs_server_dbus_s *)lbs_server;

	int ret = LBS_SERVER_ERROR_NONE;

	g_bus_unown_name(handle->owner_id);

	if (handle->prev_owner) {
		g_free(handle->prev_owner);
		handle->prev_owner = NULL;
	}

	LbsManager *lbs_mgr = NULL;
	lbs_mgr = lbs_object_get_manager(LBS_OBJECT(handle->obj_skeleton));
	g_return_val_if_fail(lbs_mgr, LBS_SERVER_ERROR_PARAMETER);

	if (handle->get_providerinfo_h) {
		g_signal_handler_disconnect(lbs_mgr, handle->get_providerinfo_h);
		handle->get_providerinfo_h = 0;
	}

	if (handle->get_status_h) {
		g_signal_handler_disconnect(lbs_mgr, handle->get_status_h);
		handle->get_status_h = 0;
	}

	if (handle->set_option_h) {
		g_signal_handler_disconnect(lbs_mgr, handle->set_option_h);
		handle->set_option_h = 0;
	}

	if (handle->add_reference_h) {
		g_signal_handler_disconnect(lbs_mgr, handle->add_reference_h);
		handle->add_reference_h = 0;
	}

	if (handle->remove_reference_h) {
		g_signal_handler_disconnect(lbs_mgr, handle->remove_reference_h);
		handle->remove_reference_h = 0;
	}
	g_object_unref(lbs_mgr);

	LbsNmea *nmea = NULL;
	nmea = lbs_object_get_nmea(LBS_OBJECT(handle->obj_skeleton));
	g_return_val_if_fail(nmea, LBS_SERVER_ERROR_PARAMETER);

	if (handle->get_nmea_h) {
		g_signal_handler_disconnect(nmea, handle->get_nmea_h);
		handle->get_nmea_h = 0;
	}
	g_object_unref(nmea);

	/* disconnect H/W gps-geofence callbacks */
	LbsGpsGeofence *gps_geofence = NULL;
	gps_geofence = lbs_object_get_gps_geofence(LBS_OBJECT(handle->obj_skeleton));
	g_return_val_if_fail(gps_geofence, LBS_SERVER_ERROR_PARAMETER);

	if (handle->add_hw_fence_h) {
		g_signal_handler_disconnect(gps_geofence, handle->add_hw_fence_h);
		handle->add_hw_fence_h = 0;
	}

	if (handle->delete_hw_fence_h) {
		g_signal_handler_disconnect(gps_geofence, handle->delete_hw_fence_h);
		handle->delete_hw_fence_h = 0;
	}

	if (handle->pause_hw_fence_h) {
		g_signal_handler_disconnect(gps_geofence, handle->pause_hw_fence_h);
		handle->pause_hw_fence_h = 0;
	}

	if (handle->resume_hw_fence_h) {
		g_signal_handler_disconnect(gps_geofence, handle->resume_hw_fence_h);
		handle->resume_hw_fence_h = 0;
	}

	g_object_unref(gps_geofence);

	if (handle->manager) {
		if (handle->owner_changed_id) {
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

