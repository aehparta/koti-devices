from socket import *
from pyRC5 import RC5
from uuid import UUID

s = socket(AF_INET, SOCK_DGRAM)
s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
s.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
s.bind(('', 17117))

uuid_prefix = b'\0\0\0\0\0\0\0\0'

key = b'12345678\0\0\0\0\0\0\0\0'
rc5 = RC5.RC5(32, 12, key)

def rc5_decrypt(data):
    data = bytearray(data)
    enc_blocks = (data[0] & 0x30) >> 4

    if enc_blocks >= 3:
        block = rc5.decryptBlock(data[24:32])
        for i in range(8):
            data[i + 24] = block[i] ^ data[i + 16]

    if enc_blocks >= 2:
        block = rc5.decryptBlock(data[16:24])
        for i in range(8):
            data[i + 16] = block[i] ^ data[i + 8]

    if enc_blocks >= 1:
        block = rc5.decryptBlock(data[8:16])
        for i in range(8):
            data[i + 8] = block[i] ^ data[i]

    block = rc5.decryptBlock(data[2:10])
    for i in range(8):
        data[i + 2] = block[i]

    return bytes(data)

while True:
    data = bytearray(s.recv(1024))

    # clear ttl
    data[0] &= 0xfc

    to = data[1]
    enc = (data[0] & 0xc0) >> 6

    data = rc5_decrypt(data)

    print(data.hex(' '))

    uuid = UUID(bytes=uuid_prefix + data[24:32])
    print(uuid)
