from pyRC5 import RC5
from crc import crc8_dallas

def decrypt(data):
    key = b'12345678\0\0\0\0\0\0\0\0'
    data = bytearray(data)

    # decrypt if necessary
    enc_blocks = (data[6] & 0x0c) >> 2
    if enc_blocks:
        rc5 = RC5.RC5(32, 12, key)
        # third block
        if enc_blocks >= 3:
            block = rc5.decryptBlock(data[24:32])
            for i in range(8):
                data[i + 24] = block[i] ^ data[i + 16]
        # second block
        if enc_blocks >= 2:
            block = rc5.decryptBlock(data[16:24])
            for i in range(8):
                data[i + 16] = block[i] ^ data[i + 8]
        # first block
        if enc_blocks >= 1:
            block = rc5.decryptBlock(data[8:16])
            for i in range(8):
                data[i + 8] = block[i]

    # calculate dallas-style 8-bit crc
    crc_in = data[8] ^ data[7]
    data[8] = data[7]
    crc = crc8_dallas(data[8:32])
    if crc != crc_in:
        # print('crc mismatch, in:', crc_in, ', calculated:', crc)
        return None

    return bytes(data)
