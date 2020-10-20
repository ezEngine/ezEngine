#include <EditorEngineProcessPCH.h>

#include <EditorEngineProcess/EngineProcGameAppUWP.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessAppUWP.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

ezEngineProcessGameApplicationUWP::ezEngineProcessGameApplicationUWP() {}

ezEngineProcessGameApplicationUWP::~ezEngineProcessGameApplicationUWP() {}

ezUniquePtr<ezEditorEngineProcessApp> ezEngineProcessGameApplicationUWP::CreateEngineProcessApp()
{
  ezUniquePtr<ezEditorEngineProcessApp> ptr = EZ_DEFAULT_NEW(ezEditorEngineProcessAppUWP);
  m_pEngineProcessApp = static_cast<ezEditorEngineProcessAppUWP*>(ptr.Borrow());
  return ptr;
}

void ezEngineProcessGameApplicationUWP::Init_ConfigureInput()
{
  ezEngineProcessGameApplication::Init_ConfigureInput();

  // Set Anchor
  {
    ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_Spatial_Hand0_Pressed;
    cfg.m_bApplyTimeScaling = false;
    ezInputManager::SetInputActionConfig("RemoteProcess", "AirTap", cfg, true);
  }
}

bool ezEngineProcessGameApplicationUWP::Run_ProcessApplicationInput()
{
  return SUPER::Run_ProcessApplicationInput();
}

#endif
