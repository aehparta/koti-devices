
include $(LIBE_PATH)/init.mk

koti_nrf_SRC = $(KOTI_PATH)/common/nrf24l01p/nrf.c
koti_opt_SRC = $(KOTI_PATH)/common/opt.c

CFLAGS += -I$(KOTI_PATH)/common
CXXFLAGS += -I$(KOTI_PATH)/common

ifeq ($(TARGET),X86)
	LDFLAGS += $(KOTI_PATH)/mosquitto/lib/libmosquitto.a -lssl -lcrypto
	CFLAGS += -I$(KOTI_PATH)/mosquitto/lib -I$(KOTI_PATH)/jsmn
	CXXFLAGS += -I$(KOTI_PATH)/mosquitto/lib -I$(KOTI_PATH)/jsmn
endif
ifneq ($(filter $(libe_DEFINES),USE_HTTPD),)
	koti_SRC += $(KOTI_PATH)/common/httpd/httpd.c
	LDFLAGS += -lmicrohttpd -lb64 -lcrypto -lpcre
endif
