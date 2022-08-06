#include <EditorPluginAudioSystem/EditorPluginAudioSystemPCH.h>

#include <EditorPluginAudioSystem/Actions/AudioSystemActions.h>
//#include <EditorPluginAudioSystem/Dialogs/AudioSystemProjectSettingsDlg.moc.h>
//#include <EditorPluginAudioSystem/Preferences/AudioSystemPreferences.h>

#include <AudioSystemPlugin/Core/AudioMiddleware.h>

#include <SharedPluginAudioSystem/Middleware/AudioMiddlewareControlsManager.h>

#include <GuiFoundation/Action/ActionManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAudioSystemAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAudioSystemSliderAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezAudioSystemActions::s_hCategoryAudioSystem;
ezActionDescriptorHandle ezAudioSystemActions::s_hProjectSettings;
ezActionDescriptorHandle ezAudioSystemActions::s_hMuteSound;
ezActionDescriptorHandle ezAudioSystemActions::s_hReloadControls;
ezActionDescriptorHandle ezAudioSystemActions::s_hMasterVolume;

void ezAudioSystemActions::RegisterActions()
{
  s_hCategoryAudioSystem = EZ_REGISTER_CATEGORY("AudioSystem");

  s_hProjectSettings = EZ_REGISTER_ACTION_1(
    "AudioSystem.Settings.Project",
    ezActionScope::Document,
    "AudioSystem",
    "",
    ezAudioSystemAction,
    ezAudioSystemAction::ActionType::ProjectSettings
  );

  s_hMuteSound = EZ_REGISTER_ACTION_1(
    "AudioSystem.Mute",
    ezActionScope::Document,
    "AudioSystem",
    "",
    ezAudioSystemAction,
    ezAudioSystemAction::ActionType::MuteSound
  );

  s_hMasterVolume = EZ_REGISTER_ACTION_1(
    "AudioSystem.MasterVolume",
    ezActionScope::Document,
    "Volume",
    "",
    ezAudioSystemSliderAction,
    ezAudioSystemSliderAction::ActionType::MasterVolume
  );

  s_hReloadControls = EZ_REGISTER_ACTION_1(
    "AudioSystem.ReloadControls",
    ezActionScope::Document,
    "AudioSystem",
    "",
    ezAudioSystemAction,
    ezAudioSystemAction::ActionType::ReloadControls
  );
}

void ezAudioSystemActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategoryAudioSystem);
  ezActionManager::UnregisterAction(s_hProjectSettings);
  ezActionManager::UnregisterAction(s_hMuteSound);
  ezActionManager::UnregisterAction(s_hReloadControls);
  ezActionManager::UnregisterAction(s_hMasterVolume);
}

void ezAudioSystemActions::MapMenuActions(const char* szMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryAudioSystem, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 9.0f);
  pMap->MapAction(s_hProjectSettings, "Menu.Editor/ProjectCategory/Menu.ProjectSettings/AudioSystem", 0.0f);

  pMap->MapAction(s_hCategoryAudioSystem, "Menu.Scene", 5.0f);
  pMap->MapAction(s_hMuteSound, "Menu.Scene/AudioSystem", 0.0f);
  pMap->MapAction(s_hReloadControls, "Menu.Scene/AudioSystem", 1.0f);
  pMap->MapAction(s_hMasterVolume, "Menu.Scene/AudioSystem", 2.0f);
}

void ezAudioSystemActions::MapToolbarActions(const char* szMapping)
{
  ezActionMap* pSceneMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pSceneMap != nullptr, "Mapping the actions failed!");

  pSceneMap->MapAction(s_hCategoryAudioSystem, "", 12.0f);
  pSceneMap->MapAction(s_hMuteSound, "AudioSystem", 0.0f);
}

ezAudioSystemAction::ezAudioSystemAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::ProjectSettings:
      SetIconPath(":/AssetIcons/Sound_Event.png");
      break;

    case ActionType::MuteSound:
    {
      SetCheckable(true);

//      ezAudioSystemProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezAudioSystemProjectPreferences>();
//      pPreferences->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezAudioSystemAction::OnPreferenceChange, this));
//
//      if (pPreferences->GetMute())
//        SetIconPath(":/Icons/SoundOff16.png");
//      else
//        SetIconPath(":/Icons/SoundOn16.png");
//
//      SetChecked(pPreferences->GetMute());
    }
    break;

    case ActionType::ReloadControls:
    {
      SetCheckable(false);
    }
    break;
  }
}

ezAudioSystemAction::~ezAudioSystemAction()
{
  if (m_Type == ActionType::MuteSound)
  {
//    ezAudioSystemProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezAudioSystemProjectPreferences>();
//    pPreferences->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezAudioSystemAction::OnPreferenceChange, this));
  }
}

void ezAudioSystemAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::ProjectSettings)
  {
//    ezQtAudioSystemProjectSettingsDlg dlg(nullptr);
//    dlg.exec();
  }

  if (m_Type == ActionType::MuteSound)
  {
//    ezAudioSystemProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezAudioSystemProjectPreferences>();
//    pPreferences->SetMute(!pPreferences->GetMute());
//
//    if (GetContext().m_pDocument)
//    {
//      GetContext().m_pDocument->ShowDocumentStatus(ezFmt("Sound is {}", pPreferences->GetMute() ? "muted" : "on"));
//    }
  }

  if (m_Type == ActionType::ReloadControls)
  {
    auto* pControlsManager = ezSingletonRegistry::GetSingletonInstance<ezAudioMiddlewareControlsManager>();
    ezLog::Info("Reload Audio Controls {}", pControlsManager->ReloadControls().Succeeded() ? "successful" : "failed");
  }
}

void ezAudioSystemAction::OnPreferenceChange(ezPreferences* pref)
{
//  ezAudioSystemProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezAudioSystemProjectPreferences>();
//
//  if (m_Type == ActionType::MuteSound)
//  {
//    if (pPreferences->GetMute())
//      SetIconPath(":/Icons/SoundOff16.png");
//    else
//      SetIconPath(":/Icons/SoundOn16.png");
//
//    SetChecked(pPreferences->GetMute());
//  }
}

//////////////////////////////////////////////////////////////////////////

ezAudioSystemSliderAction::ezAudioSystemSliderAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezSliderAction(context, szName)
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
//      ezAudioSystemProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezAudioSystemProjectPreferences>();
//
//      pPreferences->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezAudioSystemSliderAction::OnPreferenceChange, this));
//
//      SetRange(0, 20);
    }
    break;
  }

  UpdateState();
}

ezAudioSystemSliderAction::~ezAudioSystemSliderAction()
{
  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
//      ezAudioSystemProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezAudioSystemProjectPreferences>();
//      pPreferences->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezAudioSystemSliderAction::OnPreferenceChange, this));
    }
    break;
  }
}

void ezAudioSystemSliderAction::Execute(const ezVariant& value)
{
  const ezInt32 iValue = value.Get<ezInt32>();

  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
//      ezAudioSystemProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezAudioSystemProjectPreferences>();
//
//      pPreferences->SetVolume(iValue / 20.0f);
//
//      if (GetContext().m_pDocument)
//      {
//        GetContext().m_pDocument->ShowDocumentStatus(ezFmt("Sound Volume: {}%%", (int)(pPreferences->GetVolume() * 100.0f)));
//      }
    }
    break;
  }
}

void ezAudioSystemSliderAction::OnPreferenceChange(ezPreferences* pref)
{
  UpdateState();
}

void ezAudioSystemSliderAction::UpdateState()
{
  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
//      ezAudioSystemProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezAudioSystemProjectPreferences>();
//
//      SetValue(ezMath::Clamp((ezInt32)(pPreferences->GetVolume() * 20.0f), 0, 20));
    }
    break;
  }
}
