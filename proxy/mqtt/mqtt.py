import paho.mqtt.client as mqtt_client
import cfg


class MQTT:
    client = None
    prefix = None

    def __init__(self):
        self.prefix = cfg.get('mqtt.prefix', '')

        self.client = mqtt_client.Client()
        self.client.connect(cfg.get('mqtt.host', 'localhost'),
                            cfg.get('mqtt.port', 1883), 60)
        # mandatory loop for MQTT
        self.client.loop_start()

    def publish(self, topic, message):
        self.client.publish(topic=self.prefix + topic,
                            payload=message, qos=1, retain=False)
