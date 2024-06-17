#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics/Platform/PlatformFeatures.h>
#include <Foundation/System/Screen.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>

ezResult ezScreen::EnumerateScreens(ezDynamicArray<ezScreenInfo>& out_Screens)
{
  if (ANativeWindow* pWindow = ezAndroidUtils::GetAndroidApp()->window)
  {
    ezScreenInfo& currentScreen = out_Screens.ExpandAndGetRef();
    currentScreen.m_sDisplayName = "Current Display";
    currentScreen.m_iOffsetX = 0;
    currentScreen.m_iOffsetY = 0;
    currentScreen.m_iResolutionX = ANativeWindow_getWidth(pWindow);
    currentScreen.m_iResolutionY = ANativeWindow_getHeight(pWindow);
    currentScreen.m_bIsPrimary = true;
    return EZ_SUCCESS;
  }
  return EZ_FAILURE;
}
#endif
