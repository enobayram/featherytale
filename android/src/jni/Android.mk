LOCAL_PATH := $(call my-dir)

#cURL prebuilt
include $(CLEAR_VARS)
LOCAL_MODULE := curl-prebuilt
LOCAL_SRC_FILES := \
  lib/$(TARGET_ARCH_ABI)/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)
################################################################################

#traverse all the directory and subdirectory
define walk
  $(wildcard $(1)) $(foreach e, $(wildcard $(1)/*), $(call walk, $(e)))
endef

include $(CLEAR_VARS)

LOCAL_MODULE    := feathery_tale

LOCAL_CFLAGS += -std=c++11 -fexceptions -frtti
LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/src $(ORX_DIR)/scroll/include/Scroll $(LOCAL_PATH)/include

ALL_SOURCES := $(call walk, $(LOCAL_PATH)/src)

FILE_LIST := $(filter %.cpp, $(ALL_SOURCES)) $(LOCAL_PATH)/swig_gen/platform_wrapper.cpp
LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_STATIC_LIBRARIES := orx curl-prebuilt 
LOCAL_LDLIBS := -lz -llog -Wl,-s

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)

$(call import-module,lib/static/android)

