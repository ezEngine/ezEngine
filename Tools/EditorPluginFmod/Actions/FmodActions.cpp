#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginFmod/Actions/FmodActions.h>
#include <EditorPluginFmod/Dialogs/FmodProjectSettingsDlg.moc.h>
#include <Foundation/Configuration/CVar.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezActionDescriptorHandle ezFmodActions::s_hCategoryFmod;
ezActionDescriptorHandle ezFmodActions::s_hProjectSettings;
ezActionDescriptorHandle ezFmodActions::s_hMuteSound;

void ezFmodActions::RegisterActions()
{
  s_hCategoryFmod = EZ_REGISTER_CATEGORY("Fmod");
  s_hProjectSettings = EZ_REGISTER_ACTION_1("Fmod.Settings.Project", ezActionScope::Document, "Fmod", "", ezFmodAction, ezFmodAction::ActionType::ProjectSettings);
  s_hMuteSound = EZ_REGISTER_ACTION_1("Fmod.Mute", ezActionScope::Document, "Fmod", "", ezFmodAction, ezFmodAction::ActionType::MuteSound);
}

void ezFmodActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategoryFmod);
  ezActionManager::UnregisterAction(s_hProjectSettings);
  ezActionManager::UnregisterAction(s_hMuteSound);
}

void ezFmodActions::MapMenuActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentMenuBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mmapping the actions failed!");

  pMap->MapAction(s_hCategoryFmod, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 9.0f);
  pMap->MapAction(s_hProjectSettings, "Menu.Editor/ProjectCategory/Menu.ProjectSettings/Fmod", 0.0f);

  {
    ezActionMap* pSceneMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentToolBar");
    EZ_ASSERT_DEV(pSceneMap != nullptr, "Mmapping the actions failed!");

    pSceneMap->MapAction(s_hCategoryFmod, "", 6.5f);
    pSceneMap->MapAction(s_hMuteSound, "Fmod", 0.0f);
  }

  {
    ezActionMap* pSceneMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentMenuBar");
    EZ_ASSERT_DEV(pSceneMap != nullptr, "Mmapping the actions failed!");

    pSceneMap->MapAction(s_hCategoryFmod, "Menu.Scene", 5.0f);
    pSceneMap->MapAction(s_hMuteSound, "Menu.Scene/Fmod", 0.0f);
  }
}

ezFmodAction::ezFmodAction(const ezActionContext& context, const char* szName, ActionType type) : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
  case ActionType::ProjectSettings:
    SetIconPath(":/AssetIcons/Sound_Event.png");
    break;
  case ActionType::MuteSound:
    SetCheckable(true);
    
    if (ezCVarBool* cvar = static_cast<ezCVarBool*>(ezCVar::FindCVarByName("fmod_Mute")))
    {
      if (cvar->GetValue())
        SetIconPath(":/Icons/SoundOff16.png");
      else
        SetIconPath(":/Icons/SoundOn16.png");

      SetChecked(cvar->GetValue());
      cvar->m_CVarEvents.AddEventHandler(ezMakeDelegate(&ezFmodAction::CVarEventHandler, this));
    }

    break;
  }
}

ezFmodAction::~ezFmodAction()
{
  if (m_Type == ActionType::MuteSound)
  {
    if (ezCVarBool* cvar = static_cast<ezCVarBool*>(ezCVar::FindCVarByName("fmod_Mute")))
    {
      cvar->m_CVarEvents.RemoveEventHandler(ezMakeDelegate(&ezFmodAction::CVarEventHandler, this));
    }
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
    if (ezCVarBool* cvar = static_cast<ezCVarBool*>(ezCVar::FindCVarByName("fmod_Mute")))
    {
      *cvar = !cvar->GetValue();

      ezChangeCVarMsgToEngine msg;
      msg.m_sCVarName = "fmod_Mute";
      msg.m_NewValue = cvar->GetValue();

      ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
    }
  }
}

void ezFmodAction::CVarEventHandler(const ezCVar::CVarEvent& e)
{
  ezCVarBool* cvar = static_cast<ezCVarBool*>(e.m_pCVar);

  if (cvar->GetValue())
    SetIconPath(":/Icons/SoundOff16.png");
  else
    SetIconPath(":/Icons/SoundOn16.png");

  SetChecked(cvar->GetValue());
}

