import json
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

    def publish(self, topic, message, retain=False):
        self.client.publish(topic=self.prefix + topic,
                            payload=message, qos=1, retain=retain)

    def ha_config(self,
                  uuid,
                  state_topic,
                  device_class,
                  unit_of_measurement,
                  type='sensor',
                  state_class='measurement',
                  icon=None):
        unique_id = f'{uuid}_{device_class}'
        config = {
            'unique_id': unique_id,
            'name': device_class,
            'device_class': device_class,
            'unit_of_measurement': unit_of_measurement,
            'state_class': state_class,
            'state_topic': f'{self.prefix}{state_topic}',
            'device': {
                'identifiers': f'{uuid}',
                'name': f'{uuid}'
            }
        }

        if icon:
            config['icon'] = icon

        topic = f'{type}/{uuid}/{device_class}/config'
        self.publish(topic, json.dumps(config), retain=True)
