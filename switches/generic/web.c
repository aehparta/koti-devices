/*
 * Web interface stuff.
 */

#include <stdbool.h>
#include <koti.h>
#include "switch.h"
#include "web.h"

/* switch-requests */
int web_req_switch(struct MHD_Connection *connection, const char *method,
                   const char *data, size_t size,
                   const char **restr, size_t restr_c,
                   void *userdata);

int web_init(void)
{
	httpd_init();
	httpd_start(WEB_HTTPD_ADDR, WEB_HTTPD_PORT, WEB_HTTPD_PATH);
	CRIT_IF_R(httpd_register_url(NULL, "/switch/([0-9]+)/?(on|off|toggle)?/?$", web_req_switch, NULL), -1, "failed to register");
	return 0;
}

void web_quit(void)
{
	httpd_quit();
}

/* internals */

int web_req_switch(struct MHD_Connection *connection, const char *method,
                   const char *data, size_t size,
                   const char **restr, size_t restr_c,
                   void *userdata)
{
	bool state = true;
	uint8_t sw = atoi(restr[0]);

	printf("%s\n", data);

	state = switch_toggle(sw);

	struct MHD_Response *response = MHD_create_response_from_buffer(state ? 2 : 3, state ? "on" : "off", MHD_RESPMEM_MUST_COPY);
	MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
	int err = MHD_queue_response(connection, 200, response);
	MHD_destroy_response(response);

	return err;
}