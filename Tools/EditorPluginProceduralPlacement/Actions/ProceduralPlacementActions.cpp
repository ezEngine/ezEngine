#include <PCH.h>

#if 0
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginFmod/Actions/FmodActions.h>
#include <EditorPluginFmod/Dialogs/FmodProjectSettingsDlg.moc.h>
#include <Foundation/Configuration/CVar.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorPluginFmod/Preferences/FmodPreferences.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodSliderAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezActionDescriptorHandle ezFmodActions::s_hCategoryFmod;
ezActionDescriptorHandle ezFmodActions::s_hProjectSettings;
ezActionDescriptorHandle ezFmodActions::s_hMuteSound;
ezActionDescriptorHandle ezFmodActions::s_hMasterVolume;

void ezFmodActions::RegisterActions()
{
  s_hCategoryFmod = EZ_REGISTER_CATEGORY("Fmod");
  s_hProjectSettings = EZ_REGISTER_ACTION_1("Fmod.Settings.Project", ezActionScope::Document, "Fmod", "", ezFmodAction, ezFmodAction::ActionType::ProjectSettings);
  s_hMuteSound = EZ_REGISTER_ACTION_1("Fmod.Mute", ezActionScope::Document, "Fmod", "", ezFmodAction, ezFmodAction::ActionType::MuteSound);

  s_hMasterVolume = EZ_REGISTER_ACTION_1("Fmod.MasterVolume", ezActionScope::Document, "Volume", "", ezFmodSliderAction, ezFmodSliderAction::ActionType::MasterVolume);
}

void ezFmodActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategoryFmod);
  ezActionManager::UnregisterAction(s_hProjectSettings);
  ezActionManager::UnregisterAction(s_hMuteSound);
  ezActionManager::UnregisterAction(s_hMasterVolume);
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

    pSceneMap->MapAction(s_hCategoryFmod, "", 12.0f);
    pSceneMap->MapAction(s_hMuteSound, "Fmod", 0.0f);
  }

  {
    ezActionMap* pSceneMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentMenuBar");
    EZ_ASSERT_DEV(pSceneMap != nullptr, "Mmapping the actions failed!");

    pSceneMap->MapAction(s_hCategoryFmod, "Menu.Scene", 5.0f);
    pSceneMap->MapAction(s_hMuteSound, "Menu.Scene/Fmod", 0.0f);
    pSceneMap->MapAction(s_hMasterVolume, "Menu.Scene/Fmod", 1.0f);
  }
}
#endif
