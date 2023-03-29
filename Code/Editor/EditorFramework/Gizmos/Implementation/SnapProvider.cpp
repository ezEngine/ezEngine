#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Configuration/SubSystem.h>

ezAngle ezSnapProvider::s_RotationSnapValue = ezAngle::Degree(15.0f);
float ezSnapProvider::s_fScaleSnapValue = 0.125f;
float ezSnapProvider::s_fTranslationSnapValue = 0.25f;
ezEventSubscriptionID ezSnapProvider::s_UserPreferencesChanged = 0;

ezEvent<const ezSnapProviderEvent&> ezSnapProvider::s_Events;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, SnapProvider)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "EditorFrameworkMain"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezSnapProvider::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezSnapProvider::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

void ezSnapProvider::Startup()
{
  ezQtEditorApp::m_Events.AddEventHandler(ezMakeDelegate(&ezSnapProvider::EditorEventHandler));
}

void ezSnapProvider::Shutdown()
{
  if (s_UserPreferencesChanged)
  {
    ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
    pPreferences->m_ChangedEvent.RemoveEventHandler(s_UserPreferencesChanged);
  }
  ezQtEditorApp::m_Events.RemoveEventHandler(ezMakeDelegate(&ezSnapProvider::EditorEventHandler));
}

void ezSnapProvider::EditorEventHandler(const ezEditorAppEvent& e)
{
  if (e.m_Type == ezEditorAppEvent::Type::EditorStarted)
  {
    ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
    PreferenceChangedEventHandler(pPreferences);
    s_UserPreferencesChanged = pPreferences->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezSnapProvider::PreferenceChangedEventHandler));
  }
}

void ezSnapProvider::PreferenceChangedEventHandler(ezPreferences* pPreferenceBase)
{
  auto* pPreferences = static_cast<ezEditorPreferencesUser*>(pPreferenceBase);
  SetRotationSnapValue(pPreferences->m_RotationSnapValue);
  SetScaleSnapValue(pPreferences->m_fScaleSnapValue);
  SetTranslationSnapValue(pPreferences->m_fTranslationSnapValue);
}

ezAngle ezSnapProvider::GetRotationSnapValue()
{
  return s_RotationSnapValue;
}

float ezSnapProvider::GetScaleSnapValue()
{
  return s_fScaleSnapValue;
}

float ezSnapProvider::GetTranslationSnapValue()
{
  return s_fTranslationSnapValue;
}

void ezSnapProvider::SetRotationSnapValue(ezAngle angle)
{
  if (s_RotationSnapValue == angle)
    return;

  s_RotationSnapValue = angle;

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
  pPreferences->m_RotationSnapValue = angle;
  pPreferences->TriggerPreferencesChangedEvent();

  ezSnapProviderEvent e;
  e.m_Type = ezSnapProviderEvent::Type::RotationSnapChanged;
  s_Events.Broadcast(e);
}

void ezSnapProvider::SetScaleSnapValue(float fPercentage)
{
  if (s_fScaleSnapValue == fPercentage)
    return;

  s_fScaleSnapValue = fPercentage;

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
  pPreferences->m_fScaleSnapValue = fPercentage;
  pPreferences->TriggerPreferencesChangedEvent();

  ezSnapProviderEvent e;
  e.m_Type = ezSnapProviderEvent::Type::ScaleSnapChanged;
  s_Events.Broadcast(e);
}

void ezSnapProvider::SetTranslationSnapValue(float fUnits)
{
  if (s_fTranslationSnapValue == fUnits)
    return;

  s_fTranslationSnapValue = fUnits;

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
  pPreferences->m_fTranslationSnapValue = fUnits;
  pPreferences->TriggerPreferencesChangedEvent();

  ezSnapProviderEvent e;
  e.m_Type = ezSnapProviderEvent::Type::TranslationSnapChanged;
  s_Events.Broadcast(e);
}

void ezSnapProvider::SnapTranslation(ezVec3& value)
{
  if (s_fTranslationSnapValue <= 0.0f)
    return;

  value.x = ezMath::RoundToMultiple(value.x, s_fTranslationSnapValue);
  value.y = ezMath::RoundToMultiple(value.y, s_fTranslationSnapValue);
  value.z = ezMath::RoundToMultiple(value.z, s_fTranslationSnapValue);
}

void ezSnapProvider::SnapTranslationInLocalSpace(const ezQuat& qRotation, ezVec3& ref_vTranslation)
{
  if (s_fTranslationSnapValue <= 0.0f)
    return;

  const ezQuat mInvRot = -qRotation;

  ezVec3 vLocalTranslation = mInvRot * ref_vTranslation;
  vLocalTranslation.x = ezMath::RoundToMultiple(vLocalTranslation.x, s_fTranslationSnapValue);
  vLocalTranslation.y = ezMath::RoundToMultiple(vLocalTranslation.y, s_fTranslationSnapValue);
  vLocalTranslation.z = ezMath::RoundToMultiple(vLocalTranslation.z, s_fTranslationSnapValue);

  ref_vTranslation = qRotation * vLocalTranslation;
}

void ezSnapProvider::SnapRotation(ezAngle& ref_rotation)
{
  if (s_RotationSnapValue.GetRadian() != 0.0f)
  {
    ref_rotation = ezAngle::Radian(ezMath::RoundToMultiple(ref_rotation.GetRadian(), s_RotationSnapValue.GetRadian()));
  }
}

void ezSnapProvider::SnapScale(float& ref_fScale)
{
  if (s_fScaleSnapValue > 0.0f)
  {
    ref_fScale = ezMath::RoundToMultiple(ref_fScale, s_fScaleSnapValue);
  }
}

void ezSnapProvider::SnapScale(ezVec3& ref_vScale)
{
  if (s_fScaleSnapValue > 0.0f)
  {
    SnapScale(ref_vScale.x);
    SnapScale(ref_vScale.y);
    SnapScale(ref_vScale.z);
  }
}

ezVec3 ezSnapProvider::GetScaleSnapped(const ezVec3& vScale)
{
  ezVec3 res = vScale;
  SnapScale(res);
  return res;
}
