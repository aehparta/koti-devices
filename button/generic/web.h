/*
 * Web interface stuff.
 */

#ifndef _WEB_H_
#define _WEB_H_

#if defined(TARGET_X86)
#define WEB_HTTPD_ADDR "0.0.0.0"
#define WEB_HTTPD_PORT 8001
#define WEB_HTTPD_PATH "./html"
int web_init(void);
void web_quit(void);
#else
#define web_init() (0)
#define web_quit()
#endif

#endif /* _WEB_H_ */
