from socket import *
from uuid import UUID
from mqtt import MQTT
from decrypt import decrypt
import cfg

cfg = config.load()
mqtt = MQTT(cfg['mqtt'])

s = socket(AF_INET, SOCK_DGRAM)
s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
s.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
s.bind(('', 17117))

while True:
    data = s.recv(1024)
    data = decrypt(data)
    if data == None:
        continue

    print(data.hex(' '))

    mac = data[0:6]
    if mac[0] != 0x17 and mac[1] != 0x00:
        continue

    if data[10] > 0:
        print('battery', data[10], '%')

    if data[11] == 2:
        # millilitres
        ml = int.from_bytes(data[12:20], 'little')
        print('litres', ml / 1000)

    # if data[9] != 0:
    #     mqtt.publish('e7b4202e-0ced-11ec-ab05-fb55d0334872/led1/switch', 'ON')
    # else:
    #     mqtt.publish('e7b4202e-0ced-11ec-ab05-fb55d0334872/led1/switch', 'OFF')
