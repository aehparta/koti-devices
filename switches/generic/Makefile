
TARGET = X86

# generic settings
DEFINES += BUTTON_COUNT=8
USE += RC5 CRC

# x86 specific
USE_X86 += LOG LOG_COLORS HTTPD BROADCAST
LDFLAGS_X86 += -luuid

# init
include $(KOTI_PATH)/init.mk

# our own sources etc
BUILD_BINS = buttons-generic
PROGRAM_BIN = buttons-generic
buttons-generic_SRC = main.c web.c button.c $(libe_SRC) $(koti_SRC) $(koti_nrf_SRC)

# build
include $(KOTI_PATH)/build.mk

# programmable targets
include $(LIBE_PATH)/flash/program.mk

-include local.mk
