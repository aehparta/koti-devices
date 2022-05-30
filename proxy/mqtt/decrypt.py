from pyRC5 import RC5
from crc import crc8_dallas
from constants import *


def decrypt(key: bytes, data: bytes):
    key = key.ljust(16, b'\0')
    data = bytearray(data)

    # save flags and clear them
    flags = data[KOTI_NRF_PCK_HDR_FLAGS]
    data[KOTI_NRF_PCK_HDR_FLAGS] = 0

    # decrypt if necessary
    enc = flags & KOTI_NRF_FLAG_ENC_BLOCKS_MASK
    if enc >= KOTI_NRF_FLAG_ENC_RC5_1_BLOCK:
        rc5 = RC5.RC5(32, 12, key)
        # fourth block
        if enc >= KOTI_NRF_FLAG_ENC_RC5_4_BLOCKS:
            block = rc5.decryptBlock(data[24:32])
            for i in range(8):
                data[i + 24] = block[i] ^ data[i + 16]
        # third block
        if enc >= KOTI_NRF_FLAG_ENC_RC5_3_BLOCKS:
            block = rc5.decryptBlock(data[16:24])
            for i in range(8):
                data[i + 16] = block[i] ^ data[i + 8]
        # second block
        if enc >= KOTI_NRF_FLAG_ENC_RC5_2_BLOCKS:
            block = rc5.decryptBlock(data[8:16])
            for i in range(8):
                data[i + 8] = block[i] ^ data[i]
        # first block
        block = rc5.decryptBlock(data[4:12])
        for i in range(4):
            data[i + 4] = block[i] ^ data[i]
        for i in range(4):
            data[i + 8] = block[i + 4]

    # calculate dallas-style 8-bit crc
    crc_in = data[KOTI_NRF_PCK_HDR_CRC]
    data[KOTI_NRF_PCK_HDR_CRC] = 0
    crc_calculated = crc8_dallas(data)
    if crc_calculated != crc_in:
        print('crc mismatch, in:', crc_in, ', calculated:', crc_calculated)
        return (False, bytes(data[0:8]), bytes(data[8:32]))

    return (True, bytes(data[0:8]), bytes(data[8:32]))
