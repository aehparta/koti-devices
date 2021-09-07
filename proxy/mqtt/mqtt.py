import paho.mqtt.client as mqtt_client

class MQTT:
    client = None
    prefix = ''

    def __init__(self, cfg):
        self.client = mqtt_client.Client()
        self.client.connect(cfg['host'], cfg['port'], 60)
        # mandatory loop for MQTT
        self.client.loop_start()

        self.prefix = cfg['prefix']

    def publish(self, topic, message):
        self.client.publish(self.prefix + topic, payload = message)
