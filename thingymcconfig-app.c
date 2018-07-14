#include <glib.h>
#include <gio/gunixsocketaddress.h>
#include <thingymcconfig/ctrl.h>

static void sendappstate(GSocketConnection* socketconnection) {
	struct thingymcconfig_ctrl_field_index appindex = { .type =
	THINGYMCCONFIG_FIELDTYPE_APPSTATEUPDATE_APPINDEX, .index = 0 };
	struct thingymcconfig_ctrl_field_stateanderror appstate = { .type =
	THINGYMCCONFIG_FIELDTYPE_APPSTATEUPDATE_APPSTATE, .state =
	THINGYMCCONFIG_OK };

	struct thingymcconfig_ctrl_msgheader msghdr = { .type =
	THINGYMCCONFIG_MSGTYPE_EVENT_APPSTATEUPDATE, .numfields = 3 };

	GOutputStream* os = g_io_stream_get_output_stream(
			G_IO_STREAM(socketconnection));
	g_output_stream_write(os, (void*) &msghdr, sizeof(msghdr), NULL, NULL);
	g_output_stream_write(os, (void*) &appindex, sizeof(index), NULL, NULL);
	g_output_stream_write(os, (void*) &appstate, sizeof(appstate), NULL, NULL);
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
	g_input_stream_read(is, &msghdr, sizeof(msghdr), NULL, NULL);

	g_message("have message type %d with %d fields", (int) msghdr.type,
			(int) msghdr.numfields);

	int fieldcount = 0;
	gboolean terminated;

	for (int f = 0; f < msghdr.numfields; f++) {
		struct thingymcconfig_ctrl_field field;
		g_input_stream_read(is, &field, sizeof(field), NULL, NULL);
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

	return FALSE;
}

int main(int argc, char** argv) {

	GSocketAddress* socketaddress = g_unix_socket_address_new(
			"/tmp/thingymcconfig.socket");

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

	GMainLoop* mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	return 0;

	err_connect: //
	err_sock: //
	return 1;
}
