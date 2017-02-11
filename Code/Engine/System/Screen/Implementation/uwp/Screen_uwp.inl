#include <System/PCH.h>
#include <Foundation/Logging/Log.h>

#include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#include <windows.graphics.display.h>

ezResult ezScreen::EnumerateScreens(ezHybridArray<ezScreenInfo, 2>& out_Screens)
{
  out_Screens.Clear();

  ComPtr<ABI::Windows::Graphics::Display::IDisplayInformationStatics> displayInformationStatics;
  EZ_SUCCEED_OR_RETURN(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(), &displayInformationStatics));

  // Get information for current screen. Todo: How to get information for secondary screen?
  ComPtr<ABI::Windows::Graphics::Display::IDisplayInformation> currentDisplayInformation;
  EZ_SUCCEED_OR_RETURN(displayInformationStatics->GetForCurrentView(currentDisplayInformation.GetAddressOf()));
  ComPtr<ABI::Windows::Graphics::Display::IDisplayInformation4> currentDisplayInformation4;
  EZ_SUCCEED_OR_RETURN(currentDisplayInformation.As(&currentDisplayInformation4));

  ezScreenInfo& currentScreen = out_Screens.ExpandAndGetRef();
  currentScreen.m_sDisplayName = "Current Display";
  currentScreen.m_iOffsetX = 0;
  currentScreen.m_iOffsetY = 0;
  currentScreen.m_iResolutionX = 0;
  currentScreen.m_iResolutionY = 0;
  currentScreen.m_bIsPrimary = true;

  UINT rawPixelWidth, rawPixelHeight;
  EZ_SUCCEED_OR_RETURN(currentDisplayInformation4->get_ScreenHeightInRawPixels(&rawPixelHeight));
  EZ_SUCCEED_OR_RETURN(currentDisplayInformation4->get_ScreenWidthInRawPixels(&rawPixelWidth));

  currentScreen.m_iResolutionX = static_cast<ezInt32>(rawPixelWidth);
  currentScreen.m_iResolutionY = static_cast<ezInt32>(rawPixelHeight);

  return EZ_SUCCESS;
}
