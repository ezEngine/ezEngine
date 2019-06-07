#include <EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Preferences/ScenePreferences.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScenePreferencesUser, 1, ezRTTIDefaultAllocator<ezScenePreferencesUser>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ShowGrid", m_bShowGrid),
    EZ_MEMBER_PROPERTY("CameraSpeed", m_iCameraSpeed)->AddAttributes(new ezDefaultValueAttribute(10), new ezClampValueAttribute(1, 30)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezScenePreferencesUser::ezScenePreferencesUser()
    : ezPreferences(Domain::Document, "Scene")
{
  m_iCameraSpeed = 9;
}

void ezScenePreferencesUser::SetCameraSpeed(ezInt32 value)
{
  m_iCameraSpeed = ezMath::Clamp(value, 0, 24);

  // Kiff, inform the men!
  TriggerPreferencesChangedEvent();
}

void ezScenePreferencesUser::SetShowGrid(bool show)
{
  m_bShowGrid = show;

  TriggerPreferencesChangedEvent();
}
