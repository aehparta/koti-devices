/*
 * HTTP daemon.
 */

#ifndef _HTTPD_H_
#define _HTTPD_H_

#include <stdint.h>
#include <pcre.h>
#include <microhttpd.h>

int httpd_init(void);
void httpd_quit(void);
int httpd_start(char *address, uint16_t port, char *root);

int httpd_register_url(char *method_pattern, char *url_pattern,
                       int (*callback)(struct MHD_Connection *connection, const char *method,
                                       const char *data, size_t size,
                                       const char **restr, size_t restr_c,
                                       void *userdata),
                       void *userdata);

#endif /* _HTTPD_H_ */
