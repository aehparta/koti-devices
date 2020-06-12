/*
 * WiFi example.
 */

#include <libe/libe.h>
#include <math.h>
#include <esp_log.h>
#include <mdns.h>
#include <mqtt_client.h>
#include <driver/adc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "device-uuid.h"


/* only button on the PCB is connected to GPIO13 */
#define WIFI_CONFIG_BUTTON_GPIO     13
#define ERASE_CONFIG_BUTTON_GPIO    13

/* MQTT topic prefix */
#define MQTT_PREFIX                 CONFIG_MQTT_PREFIX DEVICE_UUID

/* whether or not to retain state messages */
#define MQTT_RETAIN                 1

/* QOS */
#define MQTT_QOS                    1

/* mDNS query response wait */
#ifndef DEBUG
#define MDNS_RESPONSE_WAIT_DELAY    5000
#else
#define MDNS_RESPONSE_WAIT_DELAY    1000
#endif

/* use 20kHz which above audible, some cheap PSUs tend to vibrate at the same frequency as PWM */
#define PWM_FREQUENCY               20000
#define PWM_RESOLUTION              10

/* adjust brightness down if temperature exceeds this level */
#define TEMPERATURE_LIMIT           55
/* max temperature value that we can even detect */
#define TEMPERATURE_MAX             60


esp_mqtt_client_handle_t mqtt_client;

struct pwm pwm;

struct channel {
	uint8_t pin;
	bool state;
	uint8_t value;
};
/* this setting is for chinese LED strips using GRB order */
struct channel ch[6] = { {4, 0, 0}, {2, 0, 0}, {18, 0, 0}, {19, 0, 0}, {23, 0, 0}, {22, 0, 0} };

static EventGroupHandle_t event_group;
static const int MQTT_CONNECTED_BIT = BIT0;
static const int MQTT_CONNECTING_BIT = BIT1;


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = event->client;
	char s_temp[128];

	switch (event->event_id) {
	case MQTT_EVENT_CONNECTED:
		INFO_MSG("MQTT connected");
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/led0/state", "0", 0, 1, 1);
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/led1/state", "0", 0, 1, 1);
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/led2/state", "0", 0, 1, 1);
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/led3/state", "0", 0, 1, 1);
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/led4/state", "0", 0, 1, 1);
		// esp_mqtt_client_publish(client, MQTT_PREFIX "/led5/state", "0", 0, 1, 1);


		/* first RGB channel */
		sprintf(s_temp, "%u,%u,%u", ch[0].value, ch[1].value, ch[2].value);
		esp_mqtt_client_publish(client, MQTT_PREFIX "/rgb0/state", s_temp, 0, MQTT_QOS, MQTT_RETAIN);
		esp_mqtt_client_publish(client, MQTT_PREFIX "/rgb0/switch/state", ch[0].state ? "ON" : "OFF", 0, MQTT_QOS, MQTT_RETAIN);
		esp_mqtt_client_subscribe(client, MQTT_PREFIX "/rgb0", 0);
		esp_mqtt_client_subscribe(client, MQTT_PREFIX "/rgb0/switch", 0);

		/* second RGB channel */
		sprintf(s_temp, "%u,%u,%u", ch[3].value, ch[4].value, ch[5].value);
		esp_mqtt_client_publish(client, MQTT_PREFIX "/rgb1/state", s_temp, 0, MQTT_QOS, MQTT_RETAIN);
		esp_mqtt_client_publish(client, MQTT_PREFIX "/rgb1/switch/state", ch[3].state ? "ON" : "OFF", 0, MQTT_QOS, MQTT_RETAIN);
		esp_mqtt_client_subscribe(client, MQTT_PREFIX "/rgb1", 0);
		esp_mqtt_client_subscribe(client, MQTT_PREFIX "/rgb1/switch", 0);

		/* separate channels */
		for (int i = 0; i < 6; i++) {
			char *topic = NULL;

			asprintf(&topic, MQTT_PREFIX "/led%u/state", i);
			sprintf(s_temp, "%u", ch[i].value);
			esp_mqtt_client_publish(client, topic, s_temp, 0, MQTT_QOS, MQTT_RETAIN);
			free(topic);

			asprintf(&topic, MQTT_PREFIX "/led%u/switch/state", i);
			esp_mqtt_client_publish(client, topic, ch[i].state ? "ON" : "OFF", 0, MQTT_QOS, MQTT_RETAIN);
			free(topic);

			asprintf(&topic, MQTT_PREFIX "/led%u", i);
			esp_mqtt_client_subscribe(client, topic, 0);
			free(topic);

			asprintf(&topic, MQTT_PREFIX "/led%u/switch", i);
			esp_mqtt_client_subscribe(client, topic, 0);
			free(topic);
		}

		/* now connected */
		xEventGroupSetBits(event_group, MQTT_CONNECTED_BIT);
		xEventGroupClearBits(event_group, MQTT_CONNECTING_BIT);
		break;

	case MQTT_EVENT_DISCONNECTED:
		INFO_MSG("MQTT disconnected, restarting");
		/* had trouble getting esp_mqtt_client_stop() or esp_mqtt_client_destroy() to work properly, so just restart the whole f**ing thing */
		os_restart();
		break;

	case MQTT_EVENT_DATA:
		INFO_MSG("MQTT data, %.*s: %.*s", event->topic_len, event->topic, event->data_len, event->data);

		if (strncmp(event->topic, MQTT_PREFIX "/rgb0", event->topic_len) == 0 || strncmp(event->topic, MQTT_PREFIX "/rgb1", event->topic_len) == 0) {
			int choff = event->topic[event->topic_len - 1] == '0' ? 0 : 3;
			char *vfree = strndup(event->data, event->data_len), *v = vfree;

			for (int i = 0; i < 3; i++) {
				char *b = strsep(&v, ",");
				if (b && atoi(b) >= 0 && atoi(b) <= 255) {
					ch[choff + i].value = atoi(b);
				}
			}
			free(vfree);

			for (int i = 0; i < 3; i++) {
				pwm_set_duty(&pwm, i, ch[choff].state ? ch[choff + i].value : 0);
			}

			sprintf(s_temp, "%u,%u,%u", ch[choff].value, ch[choff + 1].value, ch[choff + 2].value);
			esp_mqtt_client_publish(client, (choff ? MQTT_PREFIX "/rgb1/state" : MQTT_PREFIX "/rgb0/state"), s_temp, 0, MQTT_QOS, MQTT_RETAIN);
		} else if (strncmp(event->topic, MQTT_PREFIX "/rgb0/switch", event->topic_len) == 0 || strncmp(event->topic, MQTT_PREFIX "/rgb1/switch", event->topic_len) == 0) {
			int choff = event->topic[event->topic_len - 8] == '0' ? 0 : 3;
			bool sw = strncmp("ON", event->data, event->data_len) == 0;

			for (int i = 0; i < 3; i++) {
				ch[choff + i].state = sw;
				pwm_set_duty(&pwm, i, sw ? ch[choff + i].value : 0);
			}

			esp_mqtt_client_publish(client, (choff ? MQTT_PREFIX "/rgb1/switch/state" : MQTT_PREFIX "/rgb0/switch/state"), sw ? "ON" : "OFF", 0, MQTT_QOS, MQTT_RETAIN);
			if (sw) {
				sprintf(s_temp, "%u,%u,%u", ch[choff].value, ch[choff + 1].value, ch[choff + 2].value);
				esp_mqtt_client_publish(client, (choff ? MQTT_PREFIX "/rgb1/state" : MQTT_PREFIX "/rgb0/state"), s_temp, 0, MQTT_QOS, MQTT_RETAIN);
			}
		} else {
			int c, sw = -1;
			uint8_t value = 0;
			char *topic = NULL;

			/* try to match single channel */
			for (c = 0; c < 6; c++) {
				asprintf(&topic, MQTT_PREFIX "/led%u", c);
				if (strncmp(topic, event->topic, event->topic_len) == 0) {
					sprintf(s_temp, "%.*s", event->data_len, event->data);
					value = atoi(s_temp);
					free(topic);
					break;
				}
				free(topic);

				asprintf(&topic, MQTT_PREFIX "/led%u/switch", c);
				if (strncmp(topic, event->topic, event->topic_len) == 0) {
					sw = strncmp("ON", event->data, event->data_len) == 0 ? 1 : 0;
					free(topic);
					break;
				}
				free(topic);
			}

			/* if no match just bailout */
			if (c > 5) {
				return;
			}

			/* change value */
			if (sw > -1) {
				ch[c].state = sw;
				asprintf(&topic, MQTT_PREFIX "/led%u/switch/state", c);
				esp_mqtt_client_publish(client, topic, sw ? "ON" : "OFF", 0, MQTT_QOS, MQTT_RETAIN);
				free(topic);
			} else {
				ch[c].value = value;
			}
			if (sw != 0) {
				asprintf(&topic, MQTT_PREFIX "/led%u/state", c);
				sprintf(s_temp, "%u", ch[c].value);
				esp_mqtt_client_publish(client, topic, s_temp, 0, MQTT_QOS, MQTT_RETAIN);
				free(topic);
			}
			pwm_set_duty(&pwm, c, ch[c].state ? ch[c].value : 0);
		}
		break;

	case MQTT_EVENT_SUBSCRIBED:
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		break;
	case MQTT_EVENT_PUBLISHED:
		break;

	case MQTT_EVENT_ERROR:
		ERROR_MSG("mqtt error event");
		break;

	default:
		INFO_MSG("mqtt other event");
		break;
	}
}

char *mdns_find(char *service, char *proto, uint16_t *port)
{
	mdns_result_t *results = NULL;

	mdns_init();
	esp_err_t err = mdns_query_ptr(service, proto, MDNS_RESPONSE_WAIT_DELAY, 10,  &results);
	mdns_free();
	if (err) {
		ERROR_MSG("mDNS query failed");
		return NULL;
	}
	if (!results) {
		return NULL;
	}

	/* TODO: should really really check if result is IPv4 or IPv6 */

	/* print ip and port */
	char *host = NULL;
	asprintf(&host, IPSTR, IP2STR(&results->addr->addr.u_addr.ip4));
	*port = results->port;
	mdns_query_results_free(results);

	return host;
}

void mqtt_connect(void)
{
	/* exit if wifi is not connected */
	if (!wifi_connected() || wifi_smartconfig_in_progress()) {
		return;
	}

	/* we are now connecting, soon at least */
	xEventGroupSetBits(event_group, MQTT_CONNECTING_BIT);

	/* reset LEDs (if we were in smartconfig, they were flashing) */
	for (int i = 0; i < 6; i++) {
		pwm_set_duty(&pwm, i, ch[i].state ? ch[i].value : 0);
	}

	/* check which way we want to find broker */
	if (strlen(CONFIG_MQTT_BROKER_URL) > 0) {
		/* static mqtt server defined */
		INFO_MSG("wifi connected, using hardcoded MQTT server: %s", CONFIG_MQTT_BROKER_URL);
		esp_mqtt_client_config_t mqtt_cfg = {
			.uri = CONFIG_MQTT_BROKER_URL,
			.username = CONFIG_MQTT_USERNAME,
			.password = CONFIG_MQTT_PASSWORD
		};
		mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
		esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
		esp_mqtt_client_start(mqtt_client);
	} else {
		/* try to resolve mqtt server*/
		INFO_MSG("wifi connected, query mDNS to find mqtt server (%s.%s)", CONFIG_MQTT_MDNS_SERVICE, CONFIG_MQTT_MDNS_PROTO);
		uint16_t port = 0;
		char *host = mdns_find(CONFIG_MQTT_MDNS_SERVICE, CONFIG_MQTT_MDNS_PROTO, &port);
		/* if not found yet, try homeassistant */
		if (!host) {
			INFO_MSG("MQTT server not yet found, try homeassistant from mDNS");
			host = mdns_find("_home-assistant", "_tcp", &port);
			/* override port and try to connect to homeassistant using mqtt default port */
			port = 1883;
		}
		/* if not found, then we failed */
		if (!host) {
			ERROR_MSG("MQTT server not found by querying mDNS");
			return;
		}

		/* connect to mqtt */
		INFO_MSG("found mqtt server using mDNS: %s:%u", host, port);
		esp_mqtt_client_config_t mqtt_cfg = {
			.host = host,
			.port = port,
			.username = CONFIG_MQTT_USERNAME,
			.password = CONFIG_MQTT_PASSWORD
		};
		mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
		esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
		esp_mqtt_client_start(mqtt_client);
		free(host);
	}
}

int app_main(int argc, char *argv[])
{
	char s_temp[128];

	/* low level init: initializes some system basics depending on platform */
	os_init();
	/* debug/log init */
	log_init();
	/* nvm needed by wifi */
	nvm_init(NULL, 0);

	/* wifi connect with nvm saved values */
	wifi_init();
	wifi_connect(NULL, NULL);

	/* wifi configuration button */
	button_init(WIFI_CONFIG_BUTTON_GPIO);
	button_init(ERASE_CONFIG_BUTTON_GPIO);

	/* PWM initialization */
	pwm_open(&pwm, NULL, PWM_FREQUENCY, PWM_RESOLUTION);
	for (int i = 0; i < 6; i++) {
		pwm_channel_enable(&pwm, i, ch[i].pin);
	}

	/* adc for temperature measurement, MCP9700A is connected to ADC1 channel 5 */
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_0);

	/* event group for handling state changes */
	event_group = xEventGroupCreate();

	/* sleep a tiny amount here, this is mostly not to spam restarts when mqtt server is not available */
#ifndef DEBUG
	os_sleepi(5);
#endif

	/* start main application loop */
	os_time_t adc_read_t = os_timef();
	while (1) {
		os_wdt_reset();

		/* check for smartconfig button */
		button_pressed(WIFI_CONFIG_BUTTON_GPIO, 1000) {
			/* set repeat to zero (disable) */
			button_repeat(WIFI_CONFIG_BUTTON_GPIO, 0);
			INFO_MSG("wifi smartconfig button pressed");
			wifi_smartconfig(true);
		}
		/* check if pressed long enough, then do erase of nvm and reset */
		button_pressed(ERASE_CONFIG_BUTTON_GPIO, 5000) {
			INFO_MSG("erasing nvm and resetting device");
			nvm_erase();
			os_restart();
		}

		/* check if smartconfig in progress or if we need to start MQTT connection process */
		if (wifi_smartconfig_in_progress()) {
			/* flash all leds when in smartconfig */
			static int flasher = 0;
			for (int i = 0; i < 6; i++) {
				pwm_set_duty(&pwm, i, abs(flasher) + 2);
			}
			flasher = flasher >= 10 ? -9 : flasher + 1;
		} else if (!(xEventGroupGetBits(event_group) & (MQTT_CONNECTED_BIT | MQTT_CONNECTING_BIT)) && wifi_connected()) {
			mqtt_connect();
		}

		/* check temperature */
		if (adc_read_t < os_timef()) {
			static int t_send_counter = 0;
			static float t_mean = 0;
			/* this gets us a VERY rough reading without any calibration and noise cancellation */
			float t_now = ((float)adc1_get_raw(ADC1_CHANNEL_5) / 4096.0 * 1.1 - 0.5) * 100.0;
			t_mean += t_now;
			adc_read_t += 1;
			t_send_counter++;
			if (t_send_counter >= 60) {
				if (xEventGroupGetBits(event_group) & MQTT_CONNECTED_BIT) {
					sprintf(s_temp, "%d", (int)(t_mean / (float)t_send_counter));
					esp_mqtt_client_publish(mqtt_client, MQTT_PREFIX "/temperature", s_temp, 0, MQTT_QOS, MQTT_RETAIN);
				}
				t_send_counter = 0;
				t_mean = 0;
			}
			/* check temperature each time around and adjust PWM values lower if needed */
			if (t_now > TEMPERATURE_LIMIT) {
				WARN_MSG("temperature over limit: %.0f°C > %.0f°C", t_now, TEMPERATURE_LIMIT);

				/* each channel */
				bool rgb0_changed = false, rgb1_changed = false;
				for (int i = 0; i < 6; i++) {
					/* skip if channel is off or at zero */
					if (!ch[i].state || ch[i].value < 1) {
						continue;
					}
					/* only send changed rgb channels later */
					if (i < 3) {
						rgb0_changed = true;
					} else {
						rgb1_changed = true;
					}
					/* decrease only by one if we are under maximum */
					ch[i].value -= (t_now < TEMPERATURE_MAX ? 1 : 5);
					char *topic = NULL;
					asprintf(&topic, MQTT_PREFIX "/led%u/state", i);
					sprintf(s_temp, "%u", ch[i].value);
					esp_mqtt_client_publish(mqtt_client, topic, s_temp, 0, MQTT_QOS, MQTT_RETAIN);
					free(topic);
				}

				/* first RGB channel */
				if (rgb0_changed) {
					sprintf(s_temp, "%u,%u,%u", ch[0].value, ch[1].value, ch[2].value);
					esp_mqtt_client_publish(mqtt_client, MQTT_PREFIX "/rgb0/state", s_temp, 0, MQTT_QOS, MQTT_RETAIN);
				}

				/* second RGB channel */
				if (rgb1_changed) {
					sprintf(s_temp, "%u,%u,%u", ch[0].value, ch[1].value, ch[2].value);
					esp_mqtt_client_publish(mqtt_client, MQTT_PREFIX "/rgb1/state", s_temp, 0, MQTT_QOS, MQTT_RETAIN);
				}
			}
		}

		/* we don't need that much cpu in the main loop */
		os_delay_ms(20);
	}

	wifi_quit();
	nvm_quit();
	log_quit();
	os_quit();
	return 0;
}
