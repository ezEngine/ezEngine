#include <android/log.h>

#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "ezEngine", __VA_ARGS__)

// redirect to shared implementation
#include <Foundation/Platform/NoImpl/ConsoleWriter_NoImpl.inl>
