
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
UUID ?= $(shell uuid)
uuid:
	@echo "#define DEVICE_UUID \"$(UUID)\"" > "device-uuid.h"
	@convert -background white -fill black -font Arial -size 508x122 -pointsize 24 -gravity center label:$(UUID) device-uuid.png

# programmable targets
include $(LIBE_PATH)/flash/program.mk

-include local.mk
