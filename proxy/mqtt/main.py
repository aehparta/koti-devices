import struct
from socket import *
from uuid import UUID

import cfg
from constants import *
from decrypt import decrypt
from mqtt import MQTT

cfg.load()
mqtt = MQTT()

s = socket(AF_INET, SOCK_DGRAM)
s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
s.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
s.bind(('', 21579))


def parse_header(packet: bytes):
    src = packet[KOTI_NRF_PCK_HDR_SRC]
    dst = packet[KOTI_NRF_PCK_HDR_DST]
    uuid = UUID(int=0)
    key = bytes()

    # we are controller
    # only accept packets to controller or broadcast addresses
    if dst not in [KOTI_NRF_ADDR_CTRL, KOTI_NRF_ADDR_BROADCAST]:
        return (False, uuid, key)

    if src == KOTI_NRF_ADDR_BROADCAST:
        uuid = UUID(bytes=packet[16:32])
        key = bytes(cfg.get(f'devices.{str(uuid)}.key', ''), 'utf-8')
        if len(key) < 1:
            return (False, uuid, key)
        return (True, uuid, key)

    return (False, uuid, key)


while True:
    packet = s.recv(1024)
    (ok, uuid, key) = parse_header(packet)
    print(ok, uuid, key)
    if not ok:
        continue
    (ok, header, data) = decrypt(key, packet)
    if not ok:
        continue

    # print(packet.hex(' '))

    type = header[KOTI_NRF_PCK_HDR_TYPE]

    # power supply
    if type == KOTI_TYPE_PSU:
        (percentage, type, cells, voltage) = struct.unpack_from('<BBBxf', data, 0)
        if percentage <= 100:
            mqtt.publish(f'sensor/{uuid}/psu/percentage', percentage)
        mqtt.publish(f'sensor/{uuid}/psu/type', type)
        if cells > 0:
            mqtt.publish(f'sensor/{uuid}/psu/cells', cells)
        mqtt.publish(f'sensor/{uuid}/psu/voltage', voltage)

    # water flown
    if type in [KOTI_TYPE_WATER_FLOW_LITRE, KOTI_TYPE_WATER_FLOW_MILLILITRE]:
        (value,) = struct.unpack_from('<Q', data, 0)
        if type == KOTI_TYPE_WATER_FLOW_MILLILITRE:
            value /= 1000.0
        mqtt.publish(f'sensor/{uuid}/water/state', value)

    # temperature and humidity
    if type == KOTI_TYPE_TH:
        (t, h,) = struct.unpack_from('<ff', data, 0)
        mqtt.publish(f'sensor/{uuid}/temperature/state', t)
        mqtt.publish(f'sensor/{uuid}/humidity/state', h)

    # extremely simple click
    if type == KOTI_TYPE_CLICK:
        mqtt.publish(f'sensor/{uuid}/click/state', 'CLICK')

    # count of something
    if type == KOTI_TYPE_COUNT:
        (value,) = struct.unpack_from('<Q', data, 0)
        mqtt.publish(f'sensor/{uuid}/count/state', value)
