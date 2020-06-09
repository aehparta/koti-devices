/*
 * WiFi example.
 */

#include <libe/libe.h>
#include <esp_log.h>
#include <mdns.h>
#include <mqtt_client.h>
#include "pwm.h"
#include "device-uuid.h"


/* only button on the PCB is connected to GPIO13 */
#define WIFI_CONFIG_BUTTON_GPIO     13

/* MQTT stuff */
#define MQTT_PREFIX CONFIG_MQTT_PREFIX DEVICE_UUID


struct channel {
	bool state;
	uint8_t value;
};
struct channel ch[6] = { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} };


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = event->client;

	switch (event->event_id) {
	case MQTT_EVENT_CONNECTED:
		INFO_MSG("MQTT connected");
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/led0/state", "0", 0, 1, 1);
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/led1/state", "0", 0, 1, 1);
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/led2/state", "0", 0, 1, 1);
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/led3/state", "0", 0, 1, 1);
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/led4/state", "0", 0, 1, 1);
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/led5/state", "0", 0, 1, 1);
		esp_mqtt_client_publish(client, MQTT_PREFIX "/rgb0/state", "0,0,0", 0, 1, 1);
		esp_mqtt_client_publish(client, MQTT_PREFIX "/rgb0/switch/state", "OFF", 0, 1, 1);
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/rgb1/state", "off,0,0,0", 0, 1, 1);
		// esp_mqtt_client_subscribe(client, MQTT_PREFIX "/led0", 0);
		// esp_mqtt_client_subscribe(client, MQTT_PREFIX "/led1", 0);
		// esp_mqtt_client_subscribe(client, MQTT_PREFIX "/led2", 0);
		// esp_mqtt_client_subscribe(client, MQTT_PREFIX "/led3", 0);
		// esp_mqtt_client_subscribe(client, MQTT_PREFIX "/led4", 0);
		// esp_mqtt_client_subscribe(client, MQTT_PREFIX "/led5", 0);
		esp_mqtt_client_subscribe(client, MQTT_PREFIX "/rgb0", 0);
		esp_mqtt_client_subscribe(client, MQTT_PREFIX "/rgb0/switch", 0);
		// esp_mqtt_client_subscribe(client, MQTT_PREFIX "/rgb1", 0);
		break;
	case MQTT_EVENT_DISCONNECTED:
		/* reconnect??? */
		ERROR_MSG("MQTT disconnected");
		break;
	case MQTT_EVENT_SUBSCRIBED:
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		break;
	case MQTT_EVENT_PUBLISHED:
		break;
	case MQTT_EVENT_DATA:
		INFO_MSG("MQTT_EVENT_DATA");
		printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
		printf("DATA=%.*s\r\n", event->data_len, event->data);
		if (strncmp(event->topic, MQTT_PREFIX "/rgb0", event->topic_len) == 0) {
			char *vfree = strndup(event->data, event->data_len), *v = vfree;
			for (int i = 0; i < 3; i++) {
				char *b = strsep(&v, ",");
				if (b && atoi(b) >= 0 && atoi(b) <= 255) {
					ch[i].value = atoi(b);
				}
			}
			free(vfree);

			for (int i = 0; i < 3; i++) {
				pwm_set_duty(i, ch[i].state ? (float)ch[i].value  * 100.0 / 255.0 : 0);
			}

			char ss[32];
			sprintf(ss, "%u,%u,%u", ch[0].value, ch[1].value, ch[2].value);
			esp_mqtt_client_publish(client, MQTT_PREFIX "/rgb0/state", ss, 0, 1, 1);
		} else if (strncmp(event->topic, MQTT_PREFIX "/rgb0/switch", event->topic_len) == 0) {
			bool sw = strncmp("ON", event->data, event->data_len) == 0;
			for (int i = 0; i < 3; i++) {
				ch[i].state = sw;
				pwm_set_duty(i, ch[i].state ? (float)ch[i].value  * 100.0 / 255.0 : 0);
			}

			esp_mqtt_client_publish(client, MQTT_PREFIX "/rgb0/switch/state", sw ? "ON" : "OFF", 0, 1, 1);
			if (sw) {
				char ss[32];
				sprintf(ss, "%u,%u,%u", ch[0].value, ch[1].value, ch[2].value);
				esp_mqtt_client_publish(client, MQTT_PREFIX "/rgb0/state", ss, 0, 1, 1);
			}
		}
		break;
	case MQTT_EVENT_ERROR:
		ERROR_MSG("MQTT error event");
		break;
	default:
		/* other events */
		break;
	}
}

void mqtt_connect_task(void *p)
{
	/* wait for wifi to connect */
	while (!wifi_connected()) {
		os_delay_ms(100);
	}

	if (strlen(CONFIG_MQTT_BROKER_URL) > 0) {
		/* static mqtt server defined */
		INFO_MSG("wifi connected, using hardcoded MQTT server: %s", CONFIG_MQTT_BROKER_URL);
		esp_mqtt_client_config_t mqtt_cfg = { .uri = CONFIG_MQTT_BROKER_URL };
		esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
		esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
		esp_mqtt_client_start(client);
	} else {
		/* try to resolve mqtt server */
		INFO_MSG("wifi connected, query mDNS to find mqtt server (%s.%s)", CONFIG_MQTT_MDNS_SERVICE, CONFIG_MQTT_MDNS_PROTO);
		mdns_result_t *results = NULL;
		mdns_init();
		esp_err_t err = mdns_query_ptr(CONFIG_MQTT_MDNS_SERVICE, CONFIG_MQTT_MDNS_PROTO, 2000, 10,  &results);
		if (err) {
			ERROR_MSG("mDNS query failed when searching MQTT server");
			vTaskDelete(NULL);
		}
		if (!results) {
			WARN_MSG("mDNS query failed to find any MQTT servers");
			vTaskDelete(NULL);
		}
		INFO_MSG("found mqtt server using mDNS: " IPSTR ":%u", IP2STR(&results->addr->addr.u_addr.ip4), results->port);

		/* connect to mqtt */
		char *host = NULL;
		asprintf(&host, IPSTR, IP2STR(&results->addr->addr.u_addr.ip4));
		esp_mqtt_client_config_t mqtt_cfg = { .host = host, .port = results->port };
		esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
		esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
		esp_mqtt_client_start(client);
		free(host);

		mdns_query_results_free(results);
	}

	vTaskDelete(NULL);
}

int app_main(int argc, char *argv[])
{
	/* low level init: initializes some system basics depending on platform */
	os_init();
	/* debug/log init */
	log_init();
	/* PWM initialization */
	pwm_init();
	/* nvm initialization before wifi */
	nvm_init(NULL, 0);
	/* wifi base init */
	wifi_init();
	/* wifi configuration button */
	button_init(WIFI_CONFIG_BUTTON_GPIO);

	/* start mqtt task */
	xTaskCreate(mqtt_connect_task, "MQTT_TASK", 4096, NULL, 3, NULL);

	while (1) {
		os_wdt_reset();
		button_pressed(WIFI_CONFIG_BUTTON_GPIO, 1000) {
			button_repeat(WIFI_CONFIG_BUTTON_GPIO, 0);
			INFO_MSG("wifi configuration button pressed");
			wifi_smartconfig(true);
		}
		/* minimum delay for not to get watchdog timeout prints */
		os_delay_ms(10);
	}

	wifi_quit();
	nvm_quit();
	log_quit();
	os_quit();
	return 0;
}
