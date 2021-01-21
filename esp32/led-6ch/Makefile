
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

# programmable targets
include $(LIBE_PATH)/flash/program.mk

-include local.mk

# generate device uuid
uuid:
	UUID=$(shell uuid); \
	echo "New UUID: $$UUID"; \
	echo "$$UUID" > "device-uuid.txt"; \
	echo "#define UUID_STRING \"$$UUID\"" > "device-uuid.h"; \
	echo "$$UUID" | tr -d '-' | fold -w2 | paste -sd' ' | sed 's/ /, 0x/g' | sed 's/^/#define UUID_ARRAY { 0x/' | sed 's/$$/ }/' >> "device-uuid.h"
