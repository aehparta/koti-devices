/*
 * Web interface stuff.
 */

#include <stdbool.h>
#include <koti.h>
#include "button.h"
#include "web.h"

static int web_req_button_count(struct MHD_Connection *connection, const char *method,
                                const char *data, size_t size,
                                const char **restr, size_t restr_c,
                                void *userdata)
{
	char value[4];
	sprintf(value, "%u", button_count());
	struct MHD_Response *response = MHD_create_response_from_buffer(strlen(value), value, MHD_RESPMEM_MUST_COPY);
	MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
	int err = MHD_queue_response(connection, 200, response);
	MHD_destroy_response(response);
	return err;
}

static int web_req_button(struct MHD_Connection *connection, const char *method,
                          const char *data, size_t size,
                          const char **restr, size_t restr_c,
                          void *userdata)
{
	bool state = false;
	uint8_t sw = atoi(restr[1]);

	if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) {
		if (strcmp(data, "toggle") == 0) {
			state = button_toggle(sw);
		} else if (strcmp(data, "on") == 0) {
			state = button_on(sw);
		} else if (strcmp(data, "off") == 0) {
			state = button_off(sw);
		}
	} else {
		state = button_state(sw);
	}

	struct MHD_Response *response = MHD_create_response_from_buffer(state ? 2 : 3, state ? "on" : "off", MHD_RESPMEM_MUST_COPY);
	MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
	int err = MHD_queue_response(connection, 200, response);
	MHD_destroy_response(response);

	return err;
}

int web_init(void)
{
	httpd_init();
	httpd_start(WEB_HTTPD_ADDR, WEB_HTTPD_PORT, WEB_HTTPD_PATH);
	CRIT_IF_R(httpd_register_url(NULL, "/button/([0-9]+)/?$", web_req_button, NULL), -1, "failed to register");
	CRIT_IF_R(httpd_register_url("GET", "/button/count/?$", web_req_button_count, NULL), -1, "failed to register");
	return 0;
}

void web_quit(void)
{
	httpd_quit();
}
