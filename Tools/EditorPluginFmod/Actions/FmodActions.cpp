#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginFmod/Actions/FmodActions.h>
#include <FmodPlugin/PluginInterface.h>
#include <Foundation/Configuration/AbstractInterfaceRegistry.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

//ezActionDescriptorHandle ezFmodActions::s_hCategoryFmod;
//ezActionDescriptorHandle ezFmodActions::s_hProjectSettings;
//ezActionDescriptorHandle ezFmodActions::s_hSceneSettings;

void ezFmodActions::RegisterActions()
{
  //s_hCategoryFmod = EZ_REGISTER_CATEGORY("Fmod");
  //s_hProjectSettings = EZ_REGISTER_ACTION_1("Fmod.Settings.Project", ezActionScope::Document, "Fmod", "", ezFmodAction, ezFmodAction::ActionType::ProjectSettings);
  //s_hSceneSettings = EZ_REGISTER_ACTION_1("Fmod.Settings.Scene", ezActionScope::Document, "Fmod", "", ezFmodAction, ezFmodAction::ActionType::SceneSettings);
}

void ezFmodActions::UnregisterActions()
{
  //ezActionManager::UnregisterAction(s_hCategoryFmod);
  //ezActionManager::UnregisterAction(s_hProjectSettings);
  //ezActionManager::UnregisterAction(s_hSceneSettings);
}

void ezFmodActions::MapMenuActions()
{
  //ezActionMap* pMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentMenuBar");
  //EZ_ASSERT_DEV(pMap != nullptr, "Mmapping the actions failed!");

  //pMap->MapAction(s_hCategoryFmod, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 1.0f);
  //pMap->MapAction(s_hProjectSettings, "Menu.Editor/ProjectCategory/Menu.ProjectSettings/Fmod", 0.0f);

  //pMap->MapAction(s_hCategoryFmod, "Menu.Scene", 1.0f);
  //pMap->MapAction(s_hSceneSettings, "Menu.Scene/Fmod", 0.0f);
}

ezFmodAction::ezFmodAction(const ezActionContext& context, const char* szName, ActionType type) : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  //switch (m_Type)
  //{
  //case ActionType::ProjectSettings:
  //  //SetIconPath(":/EditorPluginScene/Icons/GizmoNone24.png"); /// \todo Icon
  //  break;
  //case ActionType::SceneSettings:
  //  //SetIconPath(":/EditorPluginScene/Icons/GizmoNone24.png"); /// \todo Icon
  //  break;
  //}
}

ezFmodAction::~ezFmodAction()
{
}

void ezFmodAction::Execute(const ezVariant& value)
{
  //if (m_Type == ActionType::ProjectSettings)
  //{
  //  ezPhysxProjectSettingsDlg dlg(nullptr);
  //  dlg.exec();
  //}

  //if (m_Type == ActionType::SceneSettings)
  //{
  //  int i = 0;
  //}
}

