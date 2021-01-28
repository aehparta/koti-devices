
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <uuid/uuid.h>
#include "../linkedlist.h"
#include "httpd.h"

#define SESSION_COOKIE_NAME "SESSIONID"

#define URL_MAX_REGEX_SUBSTRINGS 30 /* should be multiple of 3 */

enum {
	HTTPD_URL_REGEX_TYPE_NORMAL = 0,
	HTTPD_URL_REGEX_TYPE_WEBSOCKET
};

struct httpd_url_regex {
	int type;

	pcre *mrex;
	pcre *urex;

	int (*callback)(struct MHD_Connection *connection,
	                const char *url, const char *method,
	                const char *upload_data, size_t upload_data_size,
	                const char **substrings, size_t substrings_c,
	                void *userdata);
	void *userdata;

	/* linkedlist handles */
	struct httpd_url_regex *prev;
	struct httpd_url_regex *next;
};

struct httpd_session {
	/* session id */
	uuid_t id;
	/* number of active connections */
	uint32_t c_active;
	/* time of last request */
	time_t t_active;

	struct httpd_session *prev;
	struct httpd_session *next;
};

struct httpd_request {
	struct httpd_session *session;
	struct MHD_PostProcessor *pp;
	struct httpd_url_regex *url;

	char *restr[URL_MAX_REGEX_SUBSTRINGS];
	int restr_c;
};

/* static internal variables */
static struct MHD_Daemon *daemon_handle;
static char *www_root = NULL;
static struct httpd_url_regex *url_first = NULL;
static struct httpd_url_regex *url_last = NULL;
static struct httpd_session *session_first = NULL;
static struct httpd_session *session_last = NULL;

/* static internal functions */
static int httpd_404(struct MHD_Connection *connection, struct httpd_request *request, int code);
static int httpd_get_default(struct MHD_Connection *connection, struct httpd_request *request, const char *url);
static struct httpd_request *httpd_request_find(struct MHD_Connection *connection, void **con_cls);
static int httpd_request_handler(void *cls, struct MHD_Connection *connection,
                                 const char *url,
                                 const char *method, const char *version,
                                 const char *upload_data,
                                 size_t *upload_data_size,
                                 void **con_cls);
static void httpd_request_completed(void *cls, struct MHD_Connection *connection,
                                    void **con_cls, enum MHD_RequestTerminationCode toe);
static int httpd_register_url_with_type(int type, char *method_pattern, char *url_pattern,
                                        int (*callback)(struct MHD_Connection *connection,
                                                        const char *url, const char *method,
                                                        const char *upload_data, size_t upload_data_size,
                                                        const char **substrings, size_t substrings_c,
                                                        void *userdata),
                                        void *userdata);
/* session related internal functions */
static struct httpd_session *httpd_session_find(struct MHD_Connection *connection);
static int httpd_session_id_to_response(struct httpd_session *session, struct MHD_Response *response);

/* globals */

int httpd_init(void)
{
	return 0;
}

void httpd_quit(void)
{
	if (daemon_handle) {
		MHD_stop_daemon(daemon_handle);
		daemon_handle = NULL;
	}
	if (www_root) {
		free(www_root);
		www_root = NULL;
	}
	LL_GET_LOOP(url_first, url_last, {
		loop_item->mrex ? pcre_free(loop_item->mrex) : NULL;
		loop_item->urex ? pcre_free(loop_item->urex) : NULL;
		free(loop_item);
	});
}

int httpd_start(char *address, uint16_t port, char *root)
{
	www_root = strdup(root);
	daemon_handle = MHD_start_daemon(MHD_USE_EPOLL_INTERNAL_THREAD | MHD_USE_ERROR_LOG | MHD_ALLOW_UPGRADE,
	                                 port, NULL, NULL,
	                                 &httpd_request_handler, NULL,
	                                 MHD_OPTION_NOTIFY_COMPLETED, &httpd_request_completed, NULL,
	                                 MHD_OPTION_END);
	if (!daemon_handle) {
		return -1;
	}

	return 0;
}

int httpd_register_url(char *method_pattern, char *url_pattern,
                       int (*callback)(struct MHD_Connection *connection,
                                       const char *url, const char *method,
                                       const char *upload_data, size_t upload_data_size,
                                       const char **substrings, size_t substrings_c,
                                       void *userdata),
                       void *userdata)
{
	return httpd_register_url_with_type(HTTPD_URL_REGEX_TYPE_NORMAL, method_pattern, url_pattern, callback, userdata);
}

/* internals */

static int httpd_404(struct MHD_Connection *connection, struct httpd_request *request, int code)
{
	int err;
	struct MHD_Response *response;
	static char *data = "<html><head><title>404</title></head><body><h1>404</h1></body></html>";
	response = MHD_create_response_from_buffer(strlen(data),
	                                           (void *)data, MHD_RESPMEM_PERSISTENT);
	MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
	httpd_session_id_to_response(request->session, response);
	err = MHD_queue_response(connection, code, response);
	MHD_destroy_response(response);
	return err;
}

static int httpd_get_default(struct MHD_Connection *connection, struct httpd_request *request, const char *url)
{
	int err = 0;
	struct MHD_Response *response;
	char *file = NULL;
	int fd = -1;
	const char *ext = NULL;
	struct stat st;

	if (strlen(url) == 1) {
		url = "/index.html";
	}
	ext = url + strlen(url) - 1;
	for (; ext > url && *ext != '.'; ext--)
		;
	ext++;

	/* check filesystem */
	if (asprintf(&file, "%s%s", www_root, url) < 0) {
		return httpd_404(connection, request, MHD_HTTP_NOT_FOUND);
	}
	fd = open(file, O_RDONLY);
	free(file);
	if (fd < 0) {
		return httpd_404(connection, request, MHD_HTTP_NOT_FOUND);
	}

	/* read file size */
	fstat(fd, &st);

	response = MHD_create_response_from_fd(st.st_size, fd);
	if (strcmp(ext, "css") == 0) {
		MHD_add_response_header(response, "Content-Type", "text/css; charset=utf-8");
	} else if (strcmp(ext, "js") == 0) {
		MHD_add_response_header(response, "Content-Type", "application/javascript; charset=utf-8");
	} else if (strcmp(ext, "json") == 0) {
		MHD_add_response_header(response, "Content-Type", "application/json; charset=utf-8");
	} else {
		MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
	}
	httpd_session_id_to_response(request->session, response);
	err = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);

	return err;
}

static struct httpd_request *httpd_request_find(struct MHD_Connection *connection, void **con_cls)
{
	struct httpd_request *request = *con_cls;
	struct httpd_session *session;

	if (request) {
		request->session->t_active = time(NULL);
		return request;
	}

	session = httpd_session_find(connection);
	if (!session) {
		return NULL;
	}

	request = malloc(sizeof(*request));
	if (!request) {
		return NULL;
	}

	memset(request, 0, sizeof(*request));
	request->session = session;
	request->session->t_active = time(NULL);
	request->session->c_active++;

	*con_cls = request;
	return request;
}

static int httpd_request_handler(void *cls, struct MHD_Connection *connection,
                                 const char *url,
                                 const char *method, const char *version,
                                 const char *upload_data,
                                 size_t *upload_data_size, void **con_cls)
{
	int ret = -1;
	struct httpd_request *request;

	/* init request if this is a new one */
	request = httpd_request_find(connection, con_cls);
	if (!request) {
		/* something failed, badly */
		return MHD_NO;
	}

	/* if this is a continued POST/PUT */
	if (request->pp) {
		printf("continued post/put %p %p %lu\n", request, upload_data, *upload_data_size);
		return MHD_YES;
	}

	/* loop through list of urls to catch */
	for (struct httpd_url_regex *u = url_first; u; u = u->next) {
		/* method mathing */
		if (u->mrex && pcre_exec(u->mrex, NULL, method, strlen(method), 0, 0, NULL, 0)) {
			continue;
		}

		/* url matching */
		if (u->urex) {
			int ov[URL_MAX_REGEX_SUBSTRINGS];
			request->restr_c = pcre_exec(u->urex, NULL, url, strlen(url), 0, 0, ov, URL_MAX_REGEX_SUBSTRINGS);
			if (request->restr_c < 0) {
				continue;
			}
			for (int i = 0; i < request->restr_c; i++) {
				request->restr[i] = strndup(url + ov[2 * i], ov[2 * i + 1] - ov[2 * i]);
			}
		}

		/* set ret to not MHD_YES or MHD_NO so the loop will continue if nothing was done */
		ret = -1;

		/* match */
		if (u->type == HTTPD_URL_REGEX_TYPE_NORMAL) {
			if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) {
				ret = MHD_YES;
				request->pp = (void *)1;
			} else {
				ret = u->callback(connection, url, method,
				                  NULL, 0,
				                  (const char **)request->restr, request->restr_c,
				                  u->userdata);
			}
		}

		/* if ret is either MHD_YES or MHD_NO, we are done */
		if (ret == MHD_YES || ret == MHD_NO) {
			break;
		}

		/* reset substrings */
		for (int i = 0; i < request->restr_c; i++) {
			free(request->restr[i]);
		}
		request->restr_c = 0;
	}

	/* if match was found */
	if (ret == MHD_YES || ret == MHD_NO) {
		return ret;
	}

	/* only GET can get through here */
	if (strcmp(method, "GET") != 0) {
		return httpd_404(connection, request, MHD_HTTP_NOT_FOUND);
	}

	/* default GET handler */
	return httpd_get_default(connection, request, url);
}

static void httpd_request_completed(void *cls,
                                    struct MHD_Connection *connection,
                                    void **con_cls,
                                    enum MHD_RequestTerminationCode toe)
{
	struct httpd_request *request = *con_cls;
	if (request) {
		printf("request completed %p\n", request);
		request->session->c_active--;
		for (int i = 0; i < request->restr_c; i++) {
			free(request->restr[i]);
		}
		free(request);
	}
}

static int httpd_register_url_with_type(int type, char *method_pattern, char *url_pattern,
                                        int (*callback)(struct MHD_Connection *connection,
                                                        const char *url, const char *method,
                                                        const char *upload_data, size_t upload_data_size,
                                                        const char **substrings, size_t substrings_c,
                                                        void *userdata),
                                        void *userdata)
{
	const char *emsg;
	int eoff;
	struct httpd_url_regex *u = malloc(sizeof(*u));
	if (!u) {
		return -1;
	}
	memset(u, 0, sizeof(*u));
	u->type = type;

	/* compile both patterns */
	if (method_pattern) {
		u->mrex = pcre_compile(method_pattern, 0, &emsg, &eoff, NULL);
		if (!u->mrex) {
			free(u);
			fprintf(stderr, "method pattern pcre_compile() failed, pattern: %s, message: %s\n", method_pattern, emsg);
			return -1;
		}
	}
	if (url_pattern) {
		u->urex = pcre_compile(url_pattern, 0, &emsg, &eoff, NULL);
		if (!u->urex) {
			free(u);
			fprintf(stderr, "url pattern pcre_compile() failed, pattern: %s, message: %s\n", url_pattern, emsg);
			return -1;
		}
	}

	/* setup callback */
	u->callback = callback;
	u->userdata = userdata;

	/* append to list */
	LL_APP(url_first, url_last, u);

	return 0;
}

static struct httpd_session *httpd_session_find(struct MHD_Connection *connection)
{
	uuid_t id;
	const char *cookie;
	struct httpd_session *session;

	cookie = MHD_lookup_connection_value(connection, MHD_COOKIE_KIND, SESSION_COOKIE_NAME);
	if (cookie) {
		printf("found cookie %s\n", cookie);
		/* must be a valid uuid */
		if (uuid_parse(cookie, id)) {
			return NULL;
		}
		/* check for existing session */
		for (session = session_first; session; session = session->next) {
			if (uuid_compare(session->id, id) == 0) {
				return session;
			}
		}
	}

	/* create new session */
	session = malloc(sizeof(*session));
	if (!session) {
		return NULL;
	}
	memset(session, 0, sizeof(*session));
	uuid_generate(session->id);

	LL_APP(session_first, session_last, session);

	return session;
}

static int httpd_session_id_to_response(struct httpd_session *session, struct MHD_Response *response)
{
	char cookie[37];
	char str[sizeof(SESSION_COOKIE_NAME) + 1 + 37];
	uuid_unparse(session->id, cookie);
	sprintf(str, "%s=%s", SESSION_COOKIE_NAME, cookie);
	return MHD_add_response_header(response, MHD_HTTP_HEADER_SET_COOKIE, str) == MHD_NO ? -1 : 0;
}
