from pyRC5 import RC5

ENC_RC5 = 0
ENC_AES128 = 1
ENC_AES128 = 2
ENC_NONE = 3

def decrypt_rc5(data, key):
    data = bytearray(data)
    # clear ttl
    data[0] &= 0xfc
    
    enc_blocks = (data[0] & 0x30) >> 4

    rc5 = RC5.RC5(32, 12, key)

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

def decrypt(data):
    enc = (data[0] & 0xc0) >> 6
    key = b'12345678\0\0\0\0\0\0\0\0'

    if enc == ENC_RC5:
        return decrypt_rc5(data, key)
