#include <PCH.h>
#include <EditorPluginFmod/Preferences/FmodPreferences.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodProjectPreferences, 1, ezRTTIDefaultAllocator<ezFmodProjectPreferences>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Mute", m_bMute),
    EZ_MEMBER_PROPERTY("Volume", m_fMasterVolume),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

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


