/*
 * Kotivo controller: MQTT
 *
 * Authors:
 *  Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <pthread.h>
#include <mosquitto.h>
#include <libe/libe.h>
#include <libe/linkedlist.h>
#include "mqtt.h"


/* internal to get incoming mqtt messages periodically from queue */
static bool mqtt_update(void *userdata);

static void mqtt_callback_connect(struct mosquitto *mqtt, void *userdata, int reason);
static void mqtt_callback_disconnect(struct mosquitto *mqtt, void *userdata, int reason);
static void mqtt_callback_message(struct mosquitto *mqtt, void *userdata, const struct mosquitto_message *mosq_msg);
static void mqtt_callback_log(struct mosquitto *mqtt, void *userdata, int level, const char *str);

/* mqtt message queue */
struct mqtt_msg {
	char *topic;
	char *data;
	size_t size;
	bool retain;
	struct mqtt_msg *prev;
	struct mqtt_msg *next;
};
static struct mqtt_msg *mqtt_in_first = NULL;
static struct mqtt_msg *mqtt_in_last = NULL;
/* queue mutex */
static pthread_mutex_t mqtt_in_lock;

/* mosquitto instance handle */
static struct mosquitto *mqtt = NULL;
static bool mqtt_connection_ok = false;


int mqtt_init(void)
{
	/* mqtt incoming queue */
	pthread_mutex_init(&mqtt_in_lock, NULL);
	mqtt_in_first = NULL;
	mqtt_in_last = NULL;

	/* initialize mosquitto MQTT library */
	CRIT_IF_R(mosquitto_lib_init(), -1, "mosquitto initialization failed");
	/* new mqtt (mosquitto) instance */
	mqtt = mosquitto_new(NULL, true, NULL);
	NULL_IF_R(mqtt, -1);
	/* configure */
	mosquitto_max_inflight_messages_set(mqtt, 1);
	/* setup callbacks */
	mosquitto_connect_callback_set(mqtt, mqtt_callback_connect);
	mosquitto_disconnect_callback_set(mqtt, mqtt_callback_disconnect);
	mosquitto_message_callback_set(mqtt, mqtt_callback_message);
	mosquitto_log_callback_set(mqtt, mqtt_callback_log);

	return 0;
}

void mqtt_quit(void)
{
	struct mqtt_msg *msg;

	/* mosquitto stop */
	mosquitto_loop_stop(mqtt, true);
	mosquitto_destroy(mqtt);
	mosquitto_lib_cleanup();

	/* free incoming queue */
	LL_GET(mqtt_in_first, mqtt_in_last, msg);
	while (msg) {
		free(msg->topic);
		free(msg->data);
		free(msg);
		LL_GET(mqtt_in_first, mqtt_in_last, msg);
	}
	pthread_mutex_destroy(&mqtt_in_lock);
}

int mqtt_connect(const char *host, uint16_t port)
{
	/* connect to broker and start as threaded */
	CRIT_IF_R(mosquitto_connect(mqtt, host, port, MQTT_DEFAULT_KEEPALIVE), -1, "mqtt connect failed, host: %s, port: %d", host, port);
	CRIT_IF_R(mosquitto_loop_start(mqtt), -1, "failed to start mosquitto handler thread");

	return 0;
}

/**
 * Send MQTT message.
 *
 * @param  topic  Topic to send this message to
 * @param  data   Null terminated string data or NULL
 * @param  retain MQTT retain
 * @return        0 on success, -1 on errors
 */
int mqtt_send(char *topic, char *data, bool retain)
{
	int size = 0;
	if (data) {
		size = strlen(data);
	}
	int err = mosquitto_publish(mqtt, NULL, topic, size, data, MQTT_DEFAULT_QOS, retain);
	if (err != MOSQ_ERR_SUCCESS) {
		ERROR_MSG("%s", mosquitto_strerror(err));
		return -1;
	}
	return 0;
}


/* internals */


static bool mqtt_update(void *userdata)
{
	/* handle incoming messages */
	while (1) {
		int err = 0;
		struct mqtt_msg *msg;
		char *topic;
		bool match;

		/* get message from queue */
		pthread_mutex_lock(&mqtt_in_lock);
		LL_GET(mqtt_in_first, mqtt_in_last, msg);
		pthread_mutex_unlock(&mqtt_in_lock);
		if (!msg) {
			break;
		}

		/* if we come here, the message was not handled */
		err = -1;

		/* free message resources */
next:
		if (err < 0 && msg->retain && msg->data) {
			ERROR_MSG("unhandled or invalid retained message, removing it, topic: %s", msg->topic);
			mqtt_send(msg->topic, NULL, true);
		}
		if (msg->data) {
			free(msg->data);
		}
		free(msg->topic);
		free(msg);
	}

	return true;
}

static void mqtt_callback_connect(struct mosquitto *mqtt, void *userdata, int reason)
{
	if (reason == 0) {
		/* connection ok */
		mqtt_connection_ok = true;
		INFO_MSG("mqtt connect ok, thread ID: %u", pthread_self());
	} else {
		ERROR_MSG("mqtt connect failed, code: %d, reason: %s", mosquitto_strerror(reason));
	}
}

static void mqtt_callback_disconnect(struct mosquitto *mqtt, void *userdata, int reason)
{
	if (reason == 0) {
		INFO_MSG("mqtt normal disconnect after requesting it");
	} else {
		WARN_MSG("mqtt disconnect, code: %d, reason: %s", reason, mosquitto_strerror(reason));
	}
	mqtt_connection_ok = false;
}

static void mqtt_callback_message(struct mosquitto *mqtt, void *userdata, const struct mosquitto_message *mosq_msg)
{
	struct mqtt_msg *msg;

	SALLOC(msg,);
	msg->topic = strdup(mosq_msg->topic);

	if (mosq_msg->payloadlen) {
		msg->data = malloc(mosq_msg->payloadlen + 1);
		memcpy(msg->data, mosq_msg->payload, mosq_msg->payloadlen);
		msg->size = mosq_msg->payloadlen;
		/* always make sure there is null terminator at the end */
		msg->data[msg->size] = '\0';
		/* save flags */
		msg->retain = mosq_msg->retain;
		// DEBUG_MSG("mqtt message, thread ID: %u, topic: %s, payload(%u): %s", pthread_self(), msg->topic, msg->size, msg->data);
	} else {
		DEBUG_MSG("mqtt message, thread ID: %u, topic: %s, payload(0): (null)", pthread_self(), msg->topic);
	}

	pthread_mutex_lock(&mqtt_in_lock);
	LL_APP(mqtt_in_first, mqtt_in_last, msg);
	pthread_mutex_unlock(&mqtt_in_lock);
}

static void mqtt_callback_log(struct mosquitto *mqtt, void *userdata, int level, const char *str)
{
	switch (level) {
	case MOSQ_LOG_DEBUG:
		// DEBUG_MSG("MQTT: %s", str);
		break;
	case MOSQ_LOG_INFO:
		INFO_MSG("MQTT: %s", str);
		break;
	case MOSQ_LOG_NOTICE:
		NOTICE_MSG("MQTT: %s", str);
		break;
	case MOSQ_LOG_WARNING:
		WARN_MSG("MQTT: %s", str);
		break;
	default:
	case MOSQ_LOG_ERR:
		ERROR_MSG("MQTT: %s", str);
		break;
	}
}
