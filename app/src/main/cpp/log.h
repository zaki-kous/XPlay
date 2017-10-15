//
// Created by 朱乾 on 17/10/10.
//

#ifndef XPLAY_LOG_H
#define XPLAY_LOG_H

#include <android/log.h>
#include <jni.h>

#define LOG_TAG "XPlay"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#endif //XPLAY_LOG_H
