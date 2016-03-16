#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginPhysX/Actions/PhysXActions.h>
#include <EditorPluginPhysX/Dialogs/PhysXProjectSettingsDlg.moc.h>
#include <PhysXPlugin/PluginInterface.h>
#include <Foundation/Configuration/AbstractInterfaceRegistry.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysXAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezActionDescriptorHandle ezPhysXActions::s_hCategoryPhysX;
ezActionDescriptorHandle ezPhysXActions::s_hProjectSettings;
ezActionDescriptorHandle ezPhysXActions::s_hSceneSettings;

void ezPhysXActions::RegisterActions()
{
  s_hCategoryPhysX = EZ_REGISTER_CATEGORY("PhysX");
  s_hProjectSettings = EZ_REGISTER_ACTION_1("PhysX.Settings.Project", ezActionScope::Document, "PhysX", "", ezPhysXAction, ezPhysXAction::ActionType::ProjectSettings);
  s_hSceneSettings = EZ_REGISTER_ACTION_1("PhysX.Settings.Scene", ezActionScope::Document, "PhysX", "", ezPhysXAction, ezPhysXAction::ActionType::SceneSettings);
}

void ezPhysXActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategoryPhysX);
  ezActionManager::UnregisterAction(s_hProjectSettings);
  ezActionManager::UnregisterAction(s_hSceneSettings);
}

void ezPhysXActions::MapMenuActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentMenuBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryPhysX, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 10.0f);
  pMap->MapAction(s_hProjectSettings, "Menu.Editor/ProjectCategory/Menu.ProjectSettings/PhysX", 1.0f);

  pMap->MapAction(s_hCategoryPhysX, "Menu.Scene", 1.0f);
  pMap->MapAction(s_hSceneSettings, "Menu.Scene/PhysX", 1.0f);
}

ezPhysXAction::ezPhysXAction(const ezActionContext& context, const char* szName, ActionType type) : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
  case ActionType::ProjectSettings:
    //SetIconPath(":/EditorPluginScene/Icons/GizmoNone24.png"); /// \todo Icon
    break;
  case ActionType::SceneSettings:
    //SetIconPath(":/EditorPluginScene/Icons/GizmoNone24.png"); /// \todo Icon
    break;
  }
}

ezPhysXAction::~ezPhysXAction()
{
}

void ezPhysXAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::ProjectSettings)
  {
    ezPhysxProjectSettingsDlg dlg(nullptr);
    dlg.exec();
  }

  if (m_Type == ActionType::SceneSettings)
  {
    int i = 0;
  }
}

