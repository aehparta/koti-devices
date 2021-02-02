
ifeq ($(TARGET),ESP32)
	EXTRA_COMPONENT_DIRS += $(KOTI_PATH)
endif

include $(LIBE_PATH)/build.mk

mosquitto:
	make -C $(KOTI_PATH)/mosquitto WITH_DOCS=no WITH_SOCKS=no WITH_STATIC_LIBRARIES=yes WITH_SHARED_LIBRARIES=no

# generate device uuid
uuid:
	@UUID=$(shell uuid); \
	echo "New UUID: $$UUID"; \
	echo "$$UUID" > "device-uuid.txt"; \
	echo "#define UUID_STRING \"$$UUID\"" > "device-uuid.h"; \
	echo "$$UUID" | tr -d '-' | fold -w2 | paste -sd' ' | sed 's/ /, 0x/g' | sed 's/^/#define UUID_ARRAY { 0x/' | sed 's/$$/ }/' >> "device-uuid.h"
