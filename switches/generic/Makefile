
TARGET = X86

# generic settings
DEFINES += SWITCHES=8
USE += 

# x86 specific
USE_X86 += LOG LOG_COLORS HTTPD

# init
include $(KOTI_PATH)/init.mk

# our own sources etc
BUILD_BINS = switch-generic
PROGRAM_BIN = switch-generic
switch-generic_SRC = main.c $(libe_SRC) $(koti_SRC)

# build
include $(KOTI_PATH)/build.mk

# programmable targets
include $(LIBE_PATH)/flash/program.mk

-include local.mk
