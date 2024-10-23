#include <android/log.h>

#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "ezEngine", __VA_ARGS__)
