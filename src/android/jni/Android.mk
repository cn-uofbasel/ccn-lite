# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

# Get path and include Variables.mk
MAKEFILE_PATH=$(abspath $(dir $(lastword $(MAKEFILE_LIST))))
include $(MAKEFILE_PATH)/../../Variables.mk

include $(CLEAR_VARS)

LOCAL_MODULE    := ccn-lite-android
LOCAL_SRC_FILES := ccn-lite-jni.c
LOCAL_LDLIBS    := -landroid
LOCAL_CFLAGS    += $(ANDROID_CFLAGS)

include $(BUILD_SHARED_LIBRARY)
