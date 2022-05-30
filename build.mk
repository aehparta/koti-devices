
ifeq ($(TARGET),ESP32)
	EXTRA_COMPONENT_DIRS += $(KOTI_PATH)
endif

include $(LIBE_PATH)/build.mk

mosquitto:
	make -C $(KOTI_PATH)/mosquitto WITH_DOCS=no WITH_SOCKS=no WITH_STATIC_LIBRARIES=yes WITH_SHARED_LIBRARIES=no

# generate device uuid
info:
	@UUID=$(shell uuid); \
	KEY=$(shell openssl rand -hex 8); \
	echo "UUID: $$UUID"; \
	echo "KEY: $$KEY"; \
	echo "$$UUID" > "uuid.txt"; \
	echo "$$KEY" > "key.txt"; \
	echo "#define UUID_STRING \"$$UUID\"" > "device.h"; \
	echo "$$UUID" | tr -d '-' | fold -w2 | paste -sd' ' | sed 's/ /, 0x/g' | sed 's/^/#define UUID_ARRAY { 0x/' | sed 's/$$/ }/' >> "device.h"; \
	echo "#define KEY_STRING \"$$KEY\"" >> "device.h";
