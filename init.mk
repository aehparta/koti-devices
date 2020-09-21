
include $(LIBE_PATH)/init.mk

koti_nrf_SRC = $(KOTI_PATH)/common/nrf24l01p/nrf.c
koti_opt_SRC = $(KOTI_PATH)/common/opt.c

CFLAGS += -I$(KOTI_PATH)/common -I$(KOTI_PATH)/mosquitto/lib -I$(KOTI_PATH)/jsmn
CXXFLAGS += -I$(KOTI_PATH)/common -I$(KOTI_PATH)/mosquitto/lib -I$(KOTI_PATH)/jsmn

ifeq ($(TARGET),X86)
	LDFLAGS += $(KOTI_PATH)/mosquitto/lib/libmosquitto.a -lssl -lcrypto
endif
