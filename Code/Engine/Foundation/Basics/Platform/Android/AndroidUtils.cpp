#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>

android_app* ezAndroidUtils::s_app;
JavaVM* ezAndroidUtils::s_vm;

void ezAndroidUtils::SetAndroidApp(android_app* app)
{
  s_app = app;
  SetAndroidJavaVM(s_app->activity->vm);
}

android_app* ezAndroidUtils::GetAndroidApp()
{
  return s_app;
}

void ezAndroidUtils::SetAndroidJavaVM(JavaVM* vm)
{
  s_vm = vm;
}

JavaVM* ezAndroidUtils::GetAndroidJavaVM()
{
  return s_vm;
}

#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_Android_AndroidUtils);
