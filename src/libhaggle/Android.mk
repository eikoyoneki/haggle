LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

# Do not build for simulator
ifneq ($(TARGET_SIMULATOR),true)

LOCAL_SRC_FILES := \
	attribute.c \
	base64.c \
	dataobject.c \
	debug.c \
	interface.c \
	metadata.c \
	metadata_xml.c \
	ipc.c \
	node.c \
	platform.c \
	sha1.c


LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../../extlibs/libxml2-2.6.31/include

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl
endif
ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

LOCAL_SHARED_LIBRARIES += libhaggle-xml2

LOCAL_CFLAGS :=-O2 -g -std=gnu99
LOCAL_CFLAGS +=-DOS_ANDROID -DDEBUG -DDEBUG_LEAKS

LOCAL_PRELINK_MODULE := false

LOCAL_LDLIBS :=-lhaggle-xml2 -lpthread

LOCAL_MODULE := libhaggle
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_LIB_UNSTRIPPED)

include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))

endif