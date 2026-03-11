#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorPluginMiniAudio/Actions/MiniAudioActions.h>
#include <EditorPluginMiniAudio/Preferences/MiniAudioPreferences.h>
#include <GuiFoundation/Action/ActionManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMiniAudioAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMiniAudioSliderAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezMiniAudioActions::s_hCategoryMiniAudio;
ezActionDescriptorHandle ezMiniAudioActions::s_hMute;
ezActionDescriptorHandle ezMiniAudioActions::s_hVolume;

void ezMiniAudioActions::RegisterActions()
{
  s_hCategoryMiniAudio = EZ_REGISTER_CATEGORY("MiniAudio");
  s_hMute = EZ_REGISTER_ACTION_1("MiniAudio.Mute", ezActionScope::Document, "MiniAudio", "", ezMiniAudioAction, ezMiniAudioAction::ActionType::Mute);
  s_hVolume = EZ_REGISTER_ACTION_1("MiniAudio.Volume", ezActionScope::Document, "MiniAudio", "", ezMiniAudioSliderAction, ezMiniAudioSliderAction::ActionType::Volume);
}

void ezMiniAudioActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategoryMiniAudio);
  ezActionManager::UnregisterAction(s_hMute);
  ezActionManager::UnregisterAction(s_hVolume);
}

void ezMiniAudioActions::MapPluginMenuActions(ezStringView sMapping)
{
  // ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  // EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");
  //  pMap->MapAction(s_hCategoryMiniAudio, "G.Plugins.Settings", 9.0f);

  // no plugin specific menu entries at the moment
}

void ezMiniAudioActions::MapMenuActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryMiniAudio, "G.Scene", 5.0f);
  pMap->MapAction(s_hMute, "G.Scene", "MiniAudio", 0.0f);
  pMap->MapAction(s_hVolume, "G.Scene", "MiniAudio", 1.0f);
}

void ezMiniAudioActions::MapToolbarActions(ezStringView sMapping)
{
  ezActionMap* pSceneMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pSceneMap != nullptr, "Mapping the actions failed!");

  pSceneMap->MapAction(s_hCategoryMiniAudio, "", 12.0f);
  pSceneMap->MapAction(s_hMute, "MiniAudio", 0.0f);
}

ezMiniAudioAction::ezMiniAudioAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::Mute:
    {
      SetCheckable(true);

      ezMiniAudioProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezMiniAudioProjectPreferences>();
      pPreferences->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezMiniAudioAction::OnPreferenceChange, this));

      if (pPreferences->GetMute())
        SetIconPath(":/Icons/SoundOff.svg");
      else
        SetIconPath(":/Icons/SoundOn.svg");

      SetChecked(pPreferences->GetMute());
    }
    break;
  }
}

ezMiniAudioAction::~ezMiniAudioAction()
{
  if (m_Type == ActionType::Mute)
  {
    ezMiniAudioProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezMiniAudioProjectPreferences>();
    pPreferences->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezMiniAudioAction::OnPreferenceChange, this));
  }
}

void ezMiniAudioAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::Mute)
  {
    ezMiniAudioProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezMiniAudioProjectPreferences>();
    pPreferences->SetMute(!pPreferences->GetMute());

    if (GetContext().m_pDocument)
    {
      GetContext().m_pDocument->ShowDocumentStatus(ezFmt("Sound is {}", pPreferences->GetMute() ? "muted" : "on"));
    }
  }
}

void ezMiniAudioAction::OnPreferenceChange(ezPreferences* pref)
{
  ezMiniAudioProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezMiniAudioProjectPreferences>();

  if (m_Type == ActionType::Mute)
  {
    if (pPreferences->GetMute())
      SetIconPath(":/Icons/SoundOff.svg");
    else
      SetIconPath(":/Icons/SoundOn.svg");

    SetChecked(pPreferences->GetMute());
  }
}

//////////////////////////////////////////////////////////////////////////

ezMiniAudioSliderAction::ezMiniAudioSliderAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezSliderAction(context, szName)
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::Volume:
    {
      ezMiniAudioProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezMiniAudioProjectPreferences>();

      pPreferences->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezMiniAudioSliderAction::OnPreferenceChange, this));

      SetRange(0, 20);
    }
    break;
  }

  UpdateState();
}

ezMiniAudioSliderAction::~ezMiniAudioSliderAction()
{
  switch (m_Type)
  {
    case ActionType::Volume:
    {
      ezMiniAudioProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezMiniAudioProjectPreferences>();
      pPreferences->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezMiniAudioSliderAction::OnPreferenceChange, this));
    }
    break;
  }
}

void ezMiniAudioSliderAction::Execute(const ezVariant& value)
{
  const ezInt32 iValue = value.Get<ezInt32>();

  switch (m_Type)
  {
    case ActionType::Volume:
    {
      ezMiniAudioProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezMiniAudioProjectPreferences>();

      pPreferences->SetVolume(iValue / 20.0f);

      if (GetContext().m_pDocument)
      {
        GetContext().m_pDocument->ShowDocumentStatus(ezFmt("Sound Volume: {}%%", (int)(pPreferences->GetVolume() * 100.0f)));
      }
    }
    break;
  }
}

void ezMiniAudioSliderAction::OnPreferenceChange(ezPreferences* pref)
{
  UpdateState();
}

void ezMiniAudioSliderAction::UpdateState()
{
  switch (m_Type)
  {
    case ActionType::Volume:
    {
      ezMiniAudioProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezMiniAudioProjectPreferences>();

      SetValue(ezMath::Clamp((ezInt32)(pPreferences->GetVolume() * 20.0f), 0, 20));
    }
    break;
  }
}
