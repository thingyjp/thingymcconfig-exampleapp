#include <glib.h>
#include <gio/gunixsocketaddress.h>
#include <thingymcconfig/ctrl.h>
#include <oping.h>

#define APPINDEX 1

static GMainLoop* mainloop;
static pingobj_t* oping;

static void sendappstate(GSocketConnection* socketconnection) {
	struct thingymcconfig_ctrl_field_index appindex = { .type =
	THINGYMCCONFIG_FIELDTYPE_APPSTATEUPDATE_APPINDEX, .index = APPINDEX };
	struct thingymcconfig_ctrl_field_stateanderror appstate = { .type =
	THINGYMCCONFIG_FIELDTYPE_APPSTATEUPDATE_APPSTATE, .state =
	THINGYMCCONFIG_OK };

	struct thingymcconfig_ctrl_msgheader msghdr = { .type =
	THINGYMCCONFIG_MSGTYPE_EVENT_APPSTATEUPDATE, .numfields = 3 };

	GOutputStream* os = g_io_stream_get_output_stream(
			G_IO_STREAM(socketconnection));
	g_output_stream_write(os, (void*) &msghdr, sizeof(msghdr), NULL, NULL);
	g_output_stream_write(os, (void*) &appindex, sizeof(appindex), NULL, NULL);
	g_output_stream_write(os, (void*) &appstate, sizeof(appstate), NULL, NULL);
	g_output_stream_write(os, (void*) &thingymcconfig_terminator,
			sizeof(thingymcconfig_terminator), NULL, NULL);
	g_output_stream_flush(os, NULL, NULL);
}

static void sendconnectivitystate(GSocketConnection* socketconnection,
		gboolean connected) {
	struct thingymcconfig_ctrl_field_index appindex = { .type =
	THINGYMCCONFIG_FIELDTYPE_APPSTATEUPDATE_APPINDEX, .index = APPINDEX };
	struct thingymcconfig_ctrl_field_stateanderror connectivitystate = { .type =
	THINGYMCCONFIG_FIELDTYPE_APPSTATEUPDATE_CONNECTIVITY, .state =
			connected ? THINGYMCCONFIG_OK : THINGYMCCONFIG_ERR };

	struct thingymcconfig_ctrl_msgheader msghdr = { .type =
	THINGYMCCONFIG_MSGTYPE_EVENT_APPSTATEUPDATE, .numfields = 3 };

	GOutputStream* os = g_io_stream_get_output_stream(
			G_IO_STREAM(socketconnection));
	g_output_stream_write(os, (void*) &msghdr, sizeof(msghdr), NULL, NULL);
	g_output_stream_write(os, (void*) &appindex, sizeof(appindex), NULL, NULL);
	g_output_stream_write(os, (void*) &connectivitystate,
			sizeof(connectivitystate), NULL, NULL);
	g_output_stream_write(os, (void*) &thingymcconfig_terminator,
			sizeof(thingymcconfig_terminator), NULL, NULL);
	g_output_stream_flush(os, NULL, NULL);
}

static gboolean ctrlsockcallback(GIOChannel *source, GIOCondition cond,
		gpointer data) {
	GSocketConnection* socketconnection = data;
	g_message("message on ctrl socket");
	GInputStream* is = g_io_stream_get_input_stream(
			G_IO_STREAM(socketconnection));

	struct thingymcconfig_ctrl_msgheader msghdr;
	if (g_input_stream_read(is, &msghdr, sizeof(msghdr), NULL, NULL)
			!= sizeof(msghdr))
		goto err;

	g_message("have message type %d with %d fields", (int) msghdr.type,
			(int) msghdr.numfields);

	int fieldcount = 0;
	gboolean terminated;

	for (int f = 0; f < msghdr.numfields; f++) {
		struct thingymcconfig_ctrl_field field;
		if (g_input_stream_read(is, &field, sizeof(field), NULL, NULL)
				!= sizeof(field))
			goto err;
		g_message("have field; type: %d, v0: %d, v1: %d, v2: %d",
				(int) field.type, (int) field.v0, (int) field.v1,
				(int) field.v2);
		fieldcount++;
		if (field.type == THINGYMCCONFIG_FIELDTYPE_TERMINATOR) {
			terminated = TRUE;
			break;
		}
	}

	if (fieldcount != msghdr.numfields)
		g_message("bad number of fields, expected %d but got %d",
				(int) msghdr.numfields, fieldcount);
	if (!terminated)
		g_message("didn't see terminator");

	return TRUE;
	err: //
	g_main_loop_quit(mainloop);
	return FALSE;
}

static gboolean checkconnectivity(gpointer user_data) {
	GSocketConnection* connection = user_data;
	ping_send(oping);
	pingobj_iter_t *iter;
	gboolean reachable = FALSE;
	for (iter = ping_iterator_get(oping); iter != NULL; iter =
			ping_iterator_next(iter)) {
		double latency;
		size_t len = sizeof(latency);
		ping_iterator_get_info(iter, PING_INFO_LATENCY, &latency, &len);
		g_message("latency %f", latency);
		if (latency > 0)
			reachable = TRUE;
	}
	sendconnectivitystate(connection, reachable);
	return G_SOURCE_CONTINUE;
}

int main(int argc, char** argv) {
	oping = ping_construct();
	ping_host_add(oping, "google.com");

	GSocketAddress* socketaddress = g_unix_socket_address_new(
			THINGYMCCONFIG_CTRLSOCKPATH);

	GSocket* sock = g_socket_new(G_SOCKET_FAMILY_UNIX, G_SOCKET_TYPE_STREAM,
			G_SOCKET_PROTOCOL_DEFAULT, NULL);
	if (sock == NULL) {
		g_message("failed to create socket");
		goto err_sock;
	}

	GError* err = NULL;
	GSocketConnection* socketconnection =
			g_socket_connection_factory_create_connection(sock);
	if (!g_socket_connection_connect(socketconnection, socketaddress, NULL,
			&err)) {
		g_message("failed to connect control socket; %s", err->message);
		goto err_connect;
	}

	GIOChannel* channel = g_io_channel_unix_new(g_socket_get_fd(sock));
	guint source = g_io_add_watch(channel, G_IO_IN, ctrlsockcallback,
			socketconnection);
	g_io_channel_unref(channel);

	sendappstate(socketconnection);

	mainloop = g_main_loop_new(NULL, FALSE);

	g_timeout_add(60 * 1000, checkconnectivity, socketconnection);

	g_main_loop_run(mainloop);

	return 0;

	err_connect: //
	err_sock: //
	return 1;
}
