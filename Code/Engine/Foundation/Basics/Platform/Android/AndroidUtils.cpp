#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>

android_app* ezAndroidUtils::s_app;
JavaVM* ezAndroidUtils::s_vm;
jobject ezAndroidUtils::s_na;
ezEvent<ezAndroidInputEvent&> ezAndroidUtils::s_InputEvent;
ezEvent<ezInt32> ezAndroidUtils::s_AppCommandEvent;

void ezAndroidUtils::SetAndroidApp(android_app* app)
{
  s_app = app;
  SetAndroidJavaVM(s_app->activity->vm);
  SetAndroidNativeActivity(s_app->activity->clazz);
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

void ezAndroidUtils::SetAndroidNativeActivity(jobject nativeActivity)
{
  s_na = nativeActivity;
}

jobject ezAndroidUtils::GetAndroidNativeActivity()
{
  return s_na;
}

#endif


