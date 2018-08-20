#include <glib.h>
#include <oping.h>
#include <thingymcconfig/client_glib.h>

static ThingyMcConfigClient* client;
static GMainLoop* mainloop;
static pingobj_t* oping;
static gboolean reachable = FALSE;
static guint timeout;

static gboolean checkconnectivity(gpointer user_data) {
	g_message("checking connectivity");
	ping_send(oping);
	pingobj_iter_t *iter;
	for (iter = ping_iterator_get(oping); iter != NULL; iter =
			ping_iterator_next(iter)) {
		double latency;
		size_t len = sizeof(latency);
		ping_iterator_get_info(iter, PING_INFO_LATENCY, &latency, &len);
		g_message("latency %f", latency);
		if (latency > 0)
			reachable = TRUE;
	}
	thingymcconfig_client_sendconnectivitystate(client, reachable);
	return G_SOURCE_CONTINUE;
}

static void daemon_connected_cb(void) {
	g_message("daemon connected");
	thingymcconfig_client_sendappstate(client);
	thingymcconfig_client_sendconnectivitystate(client, reachable);
}

static void daemon_disconnected_cb(void) {
	g_message("daemon disconnected");
}

static void supplicant_connected_cb(void) {
	g_message("supplicant connected");
	timeout = g_timeout_add(60 * 1000, checkconnectivity, NULL);
}

static void supplicant_disconnected_cb(void) {
	g_message("supplicant disconnected");
	g_source_remove(timeout);
}

int main(int argc, char** argv) {
	oping = ping_construct();
	ping_host_add(oping, "google.com");

	mainloop = g_main_loop_new(NULL, FALSE);

	client = thingymcconfig_client_new("example");
	g_signal_connect(client, THINGYMCCONFIG_DETAILEDSIGNAL_DAEMON_CONNECTED,
			daemon_connected_cb, NULL);
	g_signal_connect(client, THINGYMCCONFIG_DETAILEDSIGNAL_DAEMON_DISCONNECTED,
			daemon_disconnected_cb, NULL);
	g_signal_connect(client, THINGYMCCONFIG_DETAILEDSIGNAL_SUPPLICANT_CONNECTED,
			supplicant_connected_cb, NULL);
	g_signal_connect(client,
			THINGYMCCONFIG_DETAILEDSIGNAL_SUPPLICANT_DISCONNECTED,
			supplicant_disconnected_cb, NULL);
	thingymcconfig_client_lazyconnect(client);

	g_main_loop_run(mainloop);

	thingymcconfig_client_free(client);

	return 0;
}
