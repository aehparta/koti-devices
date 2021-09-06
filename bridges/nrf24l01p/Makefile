
# default target is x86
TARGET = X86

USE += SPI LOG LOG_COLORS GPIO BROADCAST DRIVER_NRF24L01P
DEFINES += KOTI_NRF_CHANNEL=17 KOTI_NRF_SPEED=NRF24L01P_SPEED_250k
DEFINES += KOTI_BROADCAST_PORT=17117

USE_X86 += FTDI
DEFINES_X86 += SPI_SCLK=0 SPI_MISO=2 SPI_MOSI=1 SPI_FREQUENCY=0 SPI_CONTEXT=0
DEFINES_X86 += NRF_SS=3 NRF_CE=4

# init make system
include $(LIBE_PATH)/init.mk

# our own sources etc
BUILD_BINS = koti_proxy_nrf24l01p
PROGRAM_BIN = koti_proxy_nrf24l01p
koti_proxy_nrf24l01p_SRC = main.c $(libe_SRC)

# build
include $(LIBE_PATH)/build.mk

# programmable targets
include $(LIBE_PATH)/flash/program.mk

-include local.mk
