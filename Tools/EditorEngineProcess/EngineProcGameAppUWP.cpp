#include <PCH.h>
#include <EditorEngineProcess/EngineProcGameAppUWP.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessAppUWP.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

ezEngineProcessGameApplicationUWP::ezEngineProcessGameApplicationUWP()
{

}

ezEngineProcessGameApplicationUWP::~ezEngineProcessGameApplicationUWP()
{

}

ezUniquePtr<ezEditorEngineProcessApp> ezEngineProcessGameApplicationUWP::CreateEngineProcessApp()
{
  ezUniquePtr<ezEditorEngineProcessApp> ptr = EZ_DEFAULT_NEW(ezEditorEngineProcessAppUWP);
  m_pEngineProcessApp = static_cast<ezEditorEngineProcessAppUWP*>(ptr.Borrow());
  return ptr;
}

void ezEngineProcessGameApplicationUWP::DoConfigureInput(bool bReinitialize)
{
  ezEngineProcessGameApplication::DoConfigureInput(bReinitialize);

  // Set Anchor
  {
    ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_Spatial_Hand0_Pressed;
    cfg.m_bApplyTimeScaling = false;
    ezInputManager::SetInputActionConfig("RemoteProcess", "AirTap", cfg, true);
  }
}

void ezEngineProcessGameApplicationUWP::ProcessApplicationInput()
{
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  m_pEngineProcessApp->LoadAnchor();
#endif

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  {
    const bool bHandVisible = ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand0_Tracked) == ezKeyState::Down;

    if (ezInputManager::GetInputActionState("RemoteProcess", "AirTap") == ezKeyState::Pressed)
    {
      m_HandPressTime = ezTime::Now();

      ezVec3 posP(0), posN(0), pos(0);
      ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand0_PositionPosX, &posP.x);
      ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand0_PositionPosY, &posP.y);
      ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand0_PositionPosZ, &posP.z);
      ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand0_PositionNegX, &posN.x);
      ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand0_PositionNegY, &posN.y);
      ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand0_PositionNegZ, &posN.z);
      m_vHandStartPosition = posP - posN;
    }

    if (bHandVisible && (ezInputManager::GetInputActionState("RemoteProcess", "AirTap") == ezKeyState::Released))
    {
      if (ezTime::Now() - m_HandPressTime < ezTime::Milliseconds(500))
      {
        m_pEngineProcessApp->SetAnchor(m_vHandStartPosition);
      }
    }
  }
#endif

  ezEngineProcessGameApplication::ProcessApplicationInput();
}

#endif
