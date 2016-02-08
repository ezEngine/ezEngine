#include "Main.h"
#include "GameState.h"

#include <Foundation/Logging/Log.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/GraphicsUtils.h>

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  ezInputActionConfig cfg;

  cfg = ezInputManager::GetInputActionConfig(szInputSet, szInputAction);
  cfg.m_bApplyTimeScaling = true;

  if (szKey1 != nullptr)     cfg.m_sInputSlotTrigger[0] = szKey1;
  if (szKey2 != nullptr)     cfg.m_sInputSlotTrigger[1] = szKey2;
  if (szKey3 != nullptr)     cfg.m_sInputSlotTrigger[2] = szKey3;

  ezInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}
