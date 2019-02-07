#include <EditorPluginFmodPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorPluginFmod/Preferences/FmodPreferences.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodProjectPreferences, 1, ezRTTIDefaultAllocator<ezFmodProjectPreferences>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Mute", m_bMute),
    EZ_MEMBER_PROPERTY("Volume", m_fMasterVolume)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezFmodProjectPreferences::ezFmodProjectPreferences()
    : ezPreferences(Domain::Project, "Fmod")
{
}

void ezFmodProjectPreferences::SetMute(bool mute)
{
  m_bMute = mute;

  SyncCVars();
}

void ezFmodProjectPreferences::SetVolume(float fVolume)
{
  m_fMasterVolume = ezMath::Clamp(fVolume, 0.0f, 1.0f);

  SyncCVars();
}

void ezFmodProjectPreferences::SyncCVars()
{
  TriggerPreferencesChangedEvent();

  {
    ezChangeCVarMsgToEngine msg;
    msg.m_sCVarName = "fmod_Mute";
    msg.m_NewValue = m_bMute;

    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  {
    ezChangeCVarMsgToEngine msg;
    msg.m_sCVarName = "fmod_MasterVolume";
    msg.m_NewValue = m_fMasterVolume;

    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}
