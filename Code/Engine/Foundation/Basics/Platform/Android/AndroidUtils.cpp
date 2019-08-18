#  include <FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>

android_app* ezAndroidUtils::s_app;
JNIEnv* ezAndroidUtils::s_env;

void ezAndroidUtils::SetAndroidApp(android_app* app)
{
  s_app = app;
  s_app->activity->vm->AttachCurrentThread(&s_env, NULL);
}

android_app* ezAndroidUtils::GetAndroidApp()
{
  return s_app;
}

JNIEnv* ezAndroidUtils::GetJniEnv()
{
  return s_env;
}

#endif
