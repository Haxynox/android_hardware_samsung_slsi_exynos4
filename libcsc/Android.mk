LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := libsecmm
LOCAL_COPY_HEADERS := \
	csc.h

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	csc.c

OMX_NAME := exynos

LOCAL_C_INCLUDES := \
	$(TOP)/hardware/samsung_slsi/openmax/include/khronos \
	$(TOP)/hardware/samsung_slsi/openmax/include/$(OMX_NAME) \
	$(LOCAL_PATH)/../libexynosutils
LOCAL_CFLAGS :=

LOCAL_MODULE := libcsc

LOCAL_PRELINK_MODULE := false

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libswconverter
LOCAL_SHARED_LIBRARIES := liblog libexynosutils

LOCAL_CFLAGS += -DUSE_SAMSUNG_COLORFORMAT

ifeq ($(TARGET_BOARD_PLATFORM), exynos4)
LOCAL_SRC_FILES += hwconverter_wrapper.cpp
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../libhwconverter \
LOCAL_CFLAGS += -DUSE_FIMC
LOCAL_SHARED_LIBRARIES += libfimc libhwconverter
endif 

ifeq ($(TARGET_BOARD_PLATFORM), exynos5)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include
LOCAL_CFLAGS += -DUSE_GSCALER
LOCAL_SHARED_LIBRARIES += libexynosgscaler
endif

LOCAL_CFLAGS += -DUSE_ION
LOCAL_SHARED_LIBRARIES += libion_exynos

LOCAL_CFLAGS += -DEXYNOS_OMX

include $(BUILD_SHARED_LIBRARY)
