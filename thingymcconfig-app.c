#include <glib.h>
#include <oping.h>
#include <thingymcconfig/client_glib.h>

#define APPINDEX 1

static ThingyMcConfigClient* client;
static GMainLoop* mainloop;
static pingobj_t* oping;
static gboolean reachable = FALSE;

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

static void daemonconnectedcb(void) {
	g_message("daemon connected");
	thingymcconfig_client_sendappstate(client);
	thingymcconfig_client_sendconnectivitystate(client, reachable);
}

static void supplicantdisconnectcb(void) {
	g_message("supplicant disconnected");
}

int main(int argc, char** argv) {
	oping = ping_construct();
	ping_host_add(oping, "google.com");

	mainloop = g_main_loop_new(NULL, FALSE);

	g_timeout_add(60 * 1000, checkconnectivity, NULL);

	client = thingymcconfig_client_new();
	g_signal_connect(client,
			THINGYMCCONFIG_CLIENT_SIGNAL_DAEMON "::" THINGYMCCONFIG_CLIENT_DETAIL_DAEMON_CONNECTED,
			daemonconnectedcb, NULL);
	g_signal_connect(client,
			THINGYMCCONFIG_CLIENT_SIGNAL_NETWORKSTATE "::" THINGYMCCONFIG_CLIENT_DETAIL_NETWORKSTATE_SUPPLICANTDISCONNECTED,
			supplicantdisconnectcb, NULL);
	thingymcconfig_client_lazyconnect(client);

	g_main_loop_run(mainloop);

	return 0;
}
