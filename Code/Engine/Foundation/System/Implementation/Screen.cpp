#include <Foundation/FoundationPCH.h>

#include <Foundation/System/Screen.h>

void ezScreen::PrintScreenInfo(const ezHybridArray<ezScreenInfo, 2>& screens, ezLogInterface* pLog /*= ezLog::GetThreadLocalLogSystem()*/)
{
  EZ_LOG_BLOCK(pLog, "Screens");

  ezLog::Info(pLog, "Found {0} screens", screens.GetCount());

  for (const auto& screen : screens)
  {
    ezLog::Dev(pLog, "'{0}': Offset = ({1}, {2}), Resolution = ({3}, {4}){5}", screen.m_sDisplayName, screen.m_iOffsetX, screen.m_iOffsetY, screen.m_iResolutionX, screen.m_iResolutionY, screen.m_bIsPrimary ? " (primary)" : "");
  }
}


