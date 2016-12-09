LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	LOCAL_CXXFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -DANDROID_ARM
LOCAL_ARM_MODE := arm
endif

ifeq ($(TARGET_ARCH),x86)
LOCAL_CFLAGS +=  -DANDROID_X86
endif

ifeq ($(TARGET_ARCH),mips)
LOCAL_CFLAGS += -DANDROID_MIPS
endif

CORE_DIR := ..

LOCAL_MODULE    := libretro

include $(CORE_DIR)/Makefile.common

LOCAL_SRC_FILES := $(SOURCES_CXX) 

LOCAL_CFLAGS = -O3 -DINLINE=inline -D__LIBRETRO__ -DFRONTEND_SUPPORTS_RGB565 $(INCFLAGS)
LOCAL_C_INCLUDES += $(INCFLAGS)

include $(BUILD_SHARED_LIBRARY)
