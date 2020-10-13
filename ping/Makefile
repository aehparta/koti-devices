
TARGET = ESP32

# enable spi
USE += SPI LOG LOG_COLORS GPIO DRIVER_NRF24L01P CRC RC5
USE_X86 += FTDI

DEFINES_ESP32 += SPI_CONTEXT=HSPI_HOST SPI_FREQUENCY=0
DEFINES_ESP32 += SPI_MISO=GPIO_NUM_2 SPI_MOSI=GPIO_NUM_4 SPI_SCLK=GPIO_NUM_5
DEFINES_ESP32 += NRF_SS=GPIO_NUM_15 NRF_CE=GPIO_NUM_18

# init make system
include $(KOTI_PATH)/init.mk

# our own sources etc
BUILD_BINS = ping
PROGRAM_BIN = ping
ping_SRC = main.c $(libe_SRC) $(koti_nrf_SRC)

# build
include $(KOTI_PATH)/build.mk

# programmable targets
include $(LIBE_PATH)/flash/program.mk

-include local.mk
