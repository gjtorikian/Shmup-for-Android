LOCAL_PATH := $(call my-dir)
JNI_PATH := $(LOCAL_PATH)

include $(call all-subdir-makefiles)

include $(CLEAR_VARS)
LOCAL_PATH := $(JNI_PATH)


LOCAL_MODULE := libshmup

LOCAL_CFLAGS := -DANDROID -DNATIVE_ACTIVITY -DAL_BUILD_LIBRARY -DAL_ALEXT_PROTOTYPES $(LOCAL_CFLAGS)

SHMUP_FILES := $(wildcard $(LOCAL_PATH)/../../dEngine/src/*.c) 
SHMUP_FILES := $(SHMUP_FILES:$(LOCAL_PATH)/%=%) 

ANDROID_FILES := $(wildcard $(LOCAL_PATH)/android/*.c) 
ANDROID_FILES := $(ANDROID_FILES:$(LOCAL_PATH)/%=%) 

LIBZIP_FILES := $(wildcard $(LOCAL_PATH)/../libzip/*.c) 
LIBZIP_FILES := $(LIBZIP_FILES:$(LOCAL_PATH)/%=%) 

LOCAL_C_INCLUDES := $(LOCAL_PATH)/android $(LOCAL_PATH)/src $(LOCAL_PATH)/libzip $(LOCAL_PATH)/libpng $(LOCAL_PATH)/openal/include $(LOCAL_PATH)/openal/OpenAL32/Include 
LOCAL_SRC_FILES := $(ANDROID_FILES) $(SHMUP_FILES)
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv1_CM -lGLESv2 -lOpenSLES -ldl -lz -Wl,-s

LOCAL_STATIC_LIBRARIES := libzip libpng android_native_app_glue
LOCAL_SHARED_LIBRARIES := libopenal 

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)