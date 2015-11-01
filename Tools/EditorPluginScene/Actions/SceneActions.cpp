#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneAction, ezButtonAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezActionDescriptorHandle ezSceneActions::s_hSceneCategory;
ezActionDescriptorHandle ezSceneActions::s_hUpdatePrefabs;
ezActionDescriptorHandle ezSceneActions::s_hExportScene;

void ezSceneActions::RegisterActions()
{
  s_hSceneCategory = EZ_REGISTER_CATEGORY("SceneCategory");
  s_hUpdatePrefabs = EZ_REGISTER_ACTION_1("ActionUpdatePrefabs", ezActionScope::Document, "Scene", "Ctrl+Shift+P", ezSceneAction, ezSceneAction::ActionType::UpdatePrefabs);
  s_hExportScene = EZ_REGISTER_ACTION_1("ActionExportScene", ezActionScope::Document, "Scene", "Ctrl+E", ezSceneAction, ezSceneAction::ActionType::ExportScene);
}

void ezSceneActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSceneCategory);
  ezActionManager::UnregisterAction(s_hUpdatePrefabs);
  ezActionManager::UnregisterAction(s_hExportScene);
}

void ezSceneActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/SceneCategory");

  pMap->MapAction(s_hSceneCategory, szPath, 6.0f);
  
  pMap->MapAction(s_hUpdatePrefabs, sSubPath, 1.0f);
  pMap->MapAction(s_hExportScene, sSubPath, 2.0f);
}


ezSceneAction::ezSceneAction(const ezActionContext& context, const char* szName, ezSceneAction::ActionType type) : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_pSceneDocument = static_cast<ezSceneDocument*>(context.m_pDocument);

  switch (m_Type)
  {
  case ActionType::UpdatePrefabs:
    //SetIconPath(":/GuiFoundation/Icons/Scenegraph16.png"); /// \todo icon
    break;
  case ActionType::ExportScene:
    //SetIconPath(":/GuiFoundation/Icons/Scenegraph16.png"); /// \todo icon
    break;
  }
}

ezSceneAction::~ezSceneAction()
{
}

void ezSceneAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
  case ActionType::UpdatePrefabs:
    m_pSceneDocument->UpdatePrefabs();
    return;
  case ActionType::ExportScene:
    m_pSceneDocument->TriggerExportScene();
    return;
  }
}

