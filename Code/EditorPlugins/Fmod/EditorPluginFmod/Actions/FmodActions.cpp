#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorPluginFmod/Actions/FmodActions.h>
#include <EditorPluginFmod/Dialogs/FmodProjectSettingsDlg.moc.h>
#include <EditorPluginFmod/Preferences/FmodPreferences.h>
#include <GuiFoundation/Action/ActionManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodSliderAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezFmodActions::s_hCategoryFmod;
ezActionDescriptorHandle ezFmodActions::s_hProjectSettings;
ezActionDescriptorHandle ezFmodActions::s_hMuteSound;
ezActionDescriptorHandle ezFmodActions::s_hMasterVolume;

void ezFmodActions::RegisterActions()
{
  s_hCategoryFmod = EZ_REGISTER_CATEGORY("FMOD");
  s_hProjectSettings = EZ_REGISTER_ACTION_1("FMOD.Settings.Project", ezActionScope::Document, "FMOD", "", ezFmodAction, ezFmodAction::ActionType::ProjectSettings);
  s_hMuteSound = EZ_REGISTER_ACTION_1("FMOD.Mute", ezActionScope::Document, "FMOD", "", ezFmodAction, ezFmodAction::ActionType::MuteSound);
  s_hMasterVolume = EZ_REGISTER_ACTION_1("FMOD.MasterVolume", ezActionScope::Document, "FMOD", "", ezFmodSliderAction, ezFmodSliderAction::ActionType::MasterVolume);
}

void ezFmodActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategoryFmod);
  ezActionManager::UnregisterAction(s_hProjectSettings);
  ezActionManager::UnregisterAction(s_hMuteSound);
  ezActionManager::UnregisterAction(s_hMasterVolume);
}

void ezFmodActions::MapPluginMenuActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryFmod, "G.Plugins.Settings", 9.0f);
  pMap->MapAction(s_hProjectSettings, "G.Plugins.Settings", "FMOD", 0.0f);
}

void ezFmodActions::MapMenuActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryFmod, "G.Scene", 5.0f);
  pMap->MapAction(s_hMuteSound, "G.Scene", "FMOD", 0.0f);
  pMap->MapAction(s_hMasterVolume, "G.Scene", "FMOD", 1.0f);
}

void ezFmodActions::MapToolbarActions(ezStringView sMapping)
{
  ezActionMap* pSceneMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pSceneMap != nullptr, "Mapping the actions failed!");

  pSceneMap->MapAction(s_hCategoryFmod, "", 12.0f);
  pSceneMap->MapAction(s_hMuteSound, "FMOD", 0.0f);
}

ezFmodAction::ezFmodAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::ProjectSettings:
      SetIconPath(":/AssetIcons/Sound_Event.svg");
      break;

    case ActionType::MuteSound:
    {
      SetCheckable(true);

      ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();
      pPreferences->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezFmodAction::OnPreferenceChange, this));

      if (pPreferences->GetMute())
        SetIconPath(":/Icons/SoundOff.svg");
      else
        SetIconPath(":/Icons/SoundOn.svg");

      SetChecked(pPreferences->GetMute());
    }
    break;
  }
}

ezFmodAction::~ezFmodAction()
{
  if (m_Type == ActionType::MuteSound)
  {
    ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();
    pPreferences->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezFmodAction::OnPreferenceChange, this));
  }
}

void ezFmodAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::ProjectSettings)
  {
    ezQtFmodProjectSettingsDlg dlg(nullptr);
    dlg.exec();
  }

  if (m_Type == ActionType::MuteSound)
  {
    ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();
    pPreferences->SetMute(!pPreferences->GetMute());

    if (GetContext().m_pDocument)
    {
      GetContext().m_pDocument->ShowDocumentStatus(ezFmt("Sound is {}", pPreferences->GetMute() ? "muted" : "on"));
    }
  }
}

void ezFmodAction::OnPreferenceChange(ezPreferences* pref)
{
  ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();

  if (m_Type == ActionType::MuteSound)
  {
    if (pPreferences->GetMute())
      SetIconPath(":/Icons/SoundOff.svg");
    else
      SetIconPath(":/Icons/SoundOn.svg");

    SetChecked(pPreferences->GetMute());
  }
}

//////////////////////////////////////////////////////////////////////////

ezFmodSliderAction::ezFmodSliderAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezSliderAction(context, szName)
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
      ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();

      pPreferences->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezFmodSliderAction::OnPreferenceChange, this));

      SetRange(0, 20);
    }
    break;
  }

  UpdateState();
}

ezFmodSliderAction::~ezFmodSliderAction()
{
  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
      ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();
      pPreferences->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezFmodSliderAction::OnPreferenceChange, this));
    }
    break;
  }
}

void ezFmodSliderAction::Execute(const ezVariant& value)
{
  const ezInt32 iValue = value.Get<ezInt32>();

  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
      ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();

      pPreferences->SetVolume(iValue / 20.0f);

      if (GetContext().m_pDocument)
      {
        GetContext().m_pDocument->ShowDocumentStatus(ezFmt("Sound Volume: {}%%", (int)(pPreferences->GetVolume() * 100.0f)));
      }
    }
    break;
  }
}

void ezFmodSliderAction::OnPreferenceChange(ezPreferences* pref)
{
  UpdateState();
}

void ezFmodSliderAction::UpdateState()
{
  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
      ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();

      SetValue(ezMath::Clamp((ezInt32)(pPreferences->GetVolume() * 20.0f), 0, 20));
    }
    break;
  }
}
