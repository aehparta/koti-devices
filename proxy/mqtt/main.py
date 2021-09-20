from socket import *
from uuid import UUID
from mqtt import MQTT
from decrypt import decrypt
import cfg

cfg.load()
mqtt = MQTT()

s = socket(AF_INET, SOCK_DGRAM)
s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
s.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
s.bind(('', 17117))

uuid_prefix = b'\0\0\0\0\0\0\0\0'

while True:
    data = s.recv(1024)
    data = decrypt(data)

    print(data.hex(' '))

    uuid = UUID(bytes=uuid_prefix + data[24:32])

    if data[9] != 0:
        mqtt.publish('e7b4202e-0ced-11ec-ab05-fb55d0334872/led1/switch', 'ON')
    else:
        mqtt.publish('e7b4202e-0ced-11ec-ab05-fb55d0334872/led1/switch', 'OFF')
