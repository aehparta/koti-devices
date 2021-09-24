from socket import *
from uuid import UUID
from mqtt import MQTT
from decrypt import decrypt
import cfg

cfg = cfg.load()
mqtt = MQTT()

s = socket(AF_INET, SOCK_DGRAM)
s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
s.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
s.bind(('', 17117))

while True:
    data = s.recv(1024)
    data = decrypt(data)
    # if no valid data was received
    if data == None:
        continue

    # print(data.hex(' '))

    # check first two bytes of mac, they must be 0x17 and 0x00
    mac = data[0:6]
    if mac[0] != 0x17 and mac[1] != 0x00:
        continue

    # battery state (percent)
    if data[10] > 0:
        mqtt.publish('sensor/0x%s/battery/state' % (mac.hex()), data[10])

    # water usage, millilitres
    if data[11] == 2:
        # convert to litres
        litres = int.from_bytes(data[12:20], 'little') / 1000
        mqtt.publish('sensor/0x%s/water/state' % (mac.hex()), litres)
        # mqtt.publish('water/%s/usage' % (mac.hex()), litres)

    # if data[9] != 0:
    #     mqtt.publish('e7b4202e-0ced-11ec-ab05-fb55d0334872/led1/switch', 'ON')
    # else:
    #     mqtt.publish('e7b4202e-0ced-11ec-ab05-fb55d0334872/led1/switch', 'OFF')
