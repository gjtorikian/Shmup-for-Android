#ifndef ANDROID_UTILS_H
#define ANDROID_UTILS_H

#include <assert.h>
#include <jni.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <android/log.h>
#include <EGL/egl.h>

#include <zip.h>

extern struct zip *APKArchive;

#define  LOG_TAG    "libshmup"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define printf(fmt,args...)  __android_log_print(ANDROID_LOG_INFO  ,LOG_TAG, fmt, ##args) 

/**
 * Our saved state data.
 */
struct TOUCHSTATE
{
	float		x;
	float		y;
};

void Java_com_miadzin_shmup_TouchpadNAActivity_createAssetManager(JNIEnv* env, jclass clazz, jobject aM, jstring apkPath, jstring sdcard);
jobject getAssetManager();

double now_ms();

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

void createSoundEngine();
void createBufferQueueAudioPlayer();

void initAndroidSound(JNIEnv* env, char* str);
void SND_StartSoundTrack(void);
void SND_StopSoundTrack(void);

void shutdownAudio();

char * getAndroidFilename( char *filename );

#endif