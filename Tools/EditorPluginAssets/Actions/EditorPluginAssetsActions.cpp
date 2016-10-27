#include <PCH.h>
#include <EditorPluginAssets/Actions/EditorPluginAssetsActions.h>

#include <EditorPluginAssets/SceneImport/SceneImportDlg.moc.h>

#include <EditorFramework/Actions/ProjectActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>

ezActionDescriptorHandle ezAssetPluginActions::s_hImportScene;

void ezAssetPluginActions::RegisterActions()
{
  s_hImportScene = EZ_REGISTER_ACTION_0("Project.SceneImport", ezActionScope::Global, "Project", "", ezImportAssetAction);
}

void ezAssetPluginActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hImportScene);
}

void ezAssetPluginActions::MapActions(const char * szMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hImportScene, "Menu.Tools/ToolsCategory", 3.0f);
}


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezImportAssetAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezImportAssetAction::ezImportAssetAction(const ezActionContext& context, const char* szName)
  : ezButtonAction(context, szName, false, "")
{
  SetIconPath(":/GuiFoundation/Icons/DocumentAdd16.png");

  SetEnabled(ezToolsProject::IsProjectOpen());
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezImportAssetAction::ProjectEventHandler, this));
}

ezImportAssetAction::~ezImportAssetAction()
{
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezImportAssetAction::ProjectEventHandler, this));
}

void ezImportAssetAction::ProjectEventHandler(const ezToolsProjectEvent& e)
{
  SetEnabled(ezToolsProject::IsProjectOpen());
}

void ezImportAssetAction::Execute(const ezVariant & value)
{
  ezQtSceneImportDlg dialog;
  dialog.exec();
}
