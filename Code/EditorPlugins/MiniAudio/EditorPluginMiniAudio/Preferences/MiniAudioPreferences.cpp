#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorPluginMiniAudio/Preferences/MiniAudioPreferences.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMiniAudioProjectPreferences, 1, ezRTTIDefaultAllocator<ezMiniAudioProjectPreferences>)
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

ezMiniAudioProjectPreferences::ezMiniAudioProjectPreferences()
  : ezPreferences(Domain::Project, "MiniAudio")
{
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezMiniAudioProjectPreferences::ProcessEventHandler, this));
}

ezMiniAudioProjectPreferences::~ezMiniAudioProjectPreferences()
{
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezMiniAudioProjectPreferences::ProcessEventHandler, this));
}

void ezMiniAudioProjectPreferences::SetMute(bool bMute)
{
  m_bMute = bMute;

  SyncCVars();
}

void ezMiniAudioProjectPreferences::SetVolume(float fVolume)
{
  m_fMasterVolume = ezMath::Clamp(fVolume, 0.0f, 1.0f);

  SyncCVars();
}

void ezMiniAudioProjectPreferences::SyncCVars()
{
  TriggerPreferencesChangedEvent();

  {
    ezChangeCVarMsgToEngine msg;
    msg.m_sCVarName = "MiniAudio.Mute";
    msg.m_NewValue = m_bMute;

    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  {
    ezChangeCVarMsgToEngine msg;
    msg.m_sCVarName = "MiniAudio.Volume";
    msg.m_NewValue = m_fMasterVolume;

    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void ezMiniAudioProjectPreferences::ProcessEventHandler(const ezEditorEngineProcessConnection::Event& e)
{
  if (e.m_Type == ezEditorEngineProcessConnection::Event::Type::ProcessRestarted)
  {
    SyncCVars();
  }
}
