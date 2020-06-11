
TARGET = ESP32
USE += WIFI GPIO LOG PWM

# init
include $(LIBE_PATH)/init.mk

# our own sources etc
BUILD_BINS = led6ch
PROGRAM_BIN = led6ch
led6ch_SRC = main.c $(libe_SRC)

# build
include $(LIBE_PATH)/build.mk

# (re-)generate device uuid
uuid:
	@echo "#define DEVICE_UUID \"$(shell uuid)\"" > "device-uuid.h"

# programmable targets
include $(LIBE_PATH)/flash/program.mk

-include local.mk
