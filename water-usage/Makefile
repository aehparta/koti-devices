
TARGET = PIC8
USE += SPI GPIO NRF24L01P
# USE += LOG

# pic8
MCU_PIC8 = 16F18345
F_CPU_PIC8 = 16000000
DEFINES += LOG_BAUD=38400 PIC8_LOG_TX_PPS=RA5PPS
DEFINES += SPI_HARDCODED_SS=GPIOC2 SPI_HARDCODED_MISO=GPIOB6 SPI_HARDCODED_MOSI=GPIOB5 SPI_HARDCODED_SCLK=GPIOB4
DEFINES += NRF24L01P_HARDCODED_CE=GPIOC1

# init make system
include $(LIBE_PATH)/init.mk

# our own sources etc
BUILD_BINS = wu
PROGRAM_BIN = wu
wu_SRC = main.c $(libe_SRC)

# build
include $(LIBE_PATH)/build.mk

# programmable targets
include $(LIBE_PATH)/flash/program.mk

-include local.mk
