
#include <PCH.h>

#include <System/Screen/Screen.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#include <System/Screen/Implementation/Win32/Screen_win32.inl>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <System/Screen/Implementation/uwp/Screen_uwp.inl>
#else
#error "ezScreen is not implemented on this platform"
#endif

void ezScreen::PrintScreenInfo(const ezHybridArray<ezScreenInfo, 2>& screens, ezLogInterface* pLog /*= ezLog::GetThreadLocalLogSystem()*/)
{
  EZ_LOG_BLOCK(pLog, "Screens");

  ezLog::Info(pLog, "Found {0} screens", screens.GetCount());

  for (const auto& screen : screens)
  {
    ezLog::Dev(pLog, "'{0}': Offset = ({1}, {2}), Resolution = ({3}, {4}){5}", screen.m_sDisplayName, screen.m_iOffsetX, screen.m_iOffsetY,
               screen.m_iResolutionX, screen.m_iResolutionY, screen.m_bIsPrimary ? " (primary)" : "");
  }
}


EZ_STATICLINK_FILE(System, System_Screen_Implementation_Screen);

