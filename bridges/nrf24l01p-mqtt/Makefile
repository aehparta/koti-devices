
TARGET = X86
USE += LOG LOG_COLORS SPI GPIO #DRIVER_NRF24L01P
USE_X86 += FTDI BROADCAST

DEFINES_X86 += FTDI_VID=0 FTDI_PID=0 FTDI_DESC=NULL FTDI_SERIAL=NULL
DEFINES_X86 += SPI_SCLK=0 SPI_MISO=2 SPI_MOSI=1 SPI_FREQUENCY=0 SPI_CONTEXT=0
DEFINES_X86 += NRF_SS=3 NRF_CE=4

# init
include $(KOTI_PATH)/init.mk

# our own sources etc
BUILD_BINS = mosquitto koti-bridge-nrf24l01p
PROGRAM_BIN = koti-bridge-nrf24l01p
koti-bridge-nrf24l01p_SRC = main.c mqtt.c $(libe_SRC) $(koti_nrf_SRC) $(koti_opt_SRC)

# build
include $(KOTI_PATH)/build.mk

# programmable targets
include $(LIBE_PATH)/flash/program.mk

-include local.mk
