/*
 * Kotivo controller: MQTT
 *
 * Authors:
 *  Antti Partanen <aehparta@iki.fi>
 */

#ifndef _MQTT_H_
#define _MQTT_H_

#include <stdint.h>
#include <stdbool.h>


#define MQTT_DEFAULT_HOST       "localhost"
#define MQTT_DEFAULT_PORT       1883
#define MQTT_DEFAULT_KEEPALIVE  5 /* number of seconds of silence until sending keepalive */
#define MQTT_DEFAULT_QOS        1
#define MQTT_DEFAULT_RETAIN     true
#define MQTT_DEFAULT_QOS_LOG    0


int mqtt_init(void);
int mqtt_connect(const char *host, uint16_t port);
void mqtt_quit(void);

int mqtt_send(char *topic, char *data, bool retain);


#endif /* _MQTT_H_ */

