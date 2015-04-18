# (C) IoTone, Inc. 2015
#

# set this if we want release type, ?? is this required
# TARGET_BUILD_TYPE:= release

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES += $(LOCAL_PATH)


LOCAL_CFLAGS += -W -Wall
LOCAL_CFLAGS += -fPIC -DPIC
# LOCAL_CFLAGS += -fexceptions -DXLOCALE_NOT_USED -DSOCKLEN_T=socklen_t -DBSD=0 -DNO_SSTREAM

# LOCAL_CFLAGS += -v
# LOCAL_LDLIBS := -llog -lz

ifeq ($(TARGET_BUILD_TYPE),release)
    LOCAL_CFLAGS += -O2
endif

LOCAL_MODULE:= libccn-lite

# LOCAL_STATIC_LIBRARIES := libBasicUsageEnvironment libgroupsock
LOCAL_SHARED_LIBRARIES :=

#
# Brilliant little WC trick: http://stackoverflow.com/a/12827889/796514
#
#
# This would select all sources in subdirs
#
# This would select all sources in subdirs
LOCAL_SRC_FILES := \
    $(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)/*.c) $(wildcard $(LOCAL_PATH)/*/*.c)))

include $(BUILD_SHARED_LIBRARY)