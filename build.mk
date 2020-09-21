
include $(LIBE_PATH)/build.mk

mosquitto:
	make -C $(KOTI_PATH)/mosquitto WITH_DOCS=no WITH_SOCKS=no WITH_STATIC_LIBRARIES=yes WITH_SHARED_LIBRARIES=no
