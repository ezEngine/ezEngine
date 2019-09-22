#  include <FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>

android_app* ezAndroidUtils::s_app;

void ezAndroidUtils::SetAndroidApp(android_app* app)
{
  s_app = app;
}

android_app* ezAndroidUtils::GetAndroidApp()
{
  return s_app;
}

#endif
