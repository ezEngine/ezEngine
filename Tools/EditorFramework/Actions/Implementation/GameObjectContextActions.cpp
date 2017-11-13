#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectContextAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezActionDescriptorHandle ezGameObjectContextActions::s_hCategory;
ezActionDescriptorHandle ezGameObjectContextActions::s_hPickContextScene;
ezActionDescriptorHandle ezGameObjectContextActions::s_hPickContextObject;

void ezGameObjectContextActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("GameObjectContextCategory");
  s_hPickContextScene = EZ_REGISTER_ACTION_1("GameObjectContext.PickContextScene", ezActionScope::Window, "Game Object Context", "", ezGameObjectContextAction, ezGameObjectContextAction::ActionType::PickContextScene);
  s_hPickContextObject = EZ_REGISTER_ACTION_1("GameObjectContext.PickContextObject", ezActionScope::Window, "Game Object Context", "", ezGameObjectContextAction, ezGameObjectContextAction::ActionType::PickContextObject);
}


void ezGameObjectContextActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hPickContextScene);
  ezActionManager::UnregisterAction(s_hPickContextObject);
}

void ezGameObjectContextActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 10.0f);

  const char* szSubPath = "GameObjectContextCategory";

  pMap->MapAction(s_hPickContextScene, szSubPath, 1.0f);
  pMap->MapAction(s_hPickContextObject, szSubPath, 2.0f);
}

ezGameObjectContextAction::ezGameObjectContextAction(const ezActionContext& context, const char* szName, ezGameObjectContextAction::ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
  case ActionType::PickContextScene:
    SetIconPath(":/EditorPluginAssets/PickTarget16.png");
    break;
  case ActionType::PickContextObject:
    SetIconPath(":/EditorPluginAssets/PickTarget16.png");
    break;
  }
}

ezGameObjectContextAction::~ezGameObjectContextAction()
{
}

void ezGameObjectContextAction::Execute(const ezVariant& value)
{
  ezGameObjectContextDocument* pDocument = static_cast<ezGameObjectContextDocument*>(GetContext().m_pDocument);
  ezUuid document = pDocument->GetContextDocumentGuid();
  switch (m_Type)
  {
  case ActionType::PickContextScene:
    {
      ezQtAssetBrowserDlg dlg(GetContext().m_pWindow, document, "Scene;Prefab");
      if (dlg.exec() == 0)
        return;

      document = dlg.GetSelectedAssetGuid();
      pDocument->SetContext(document, ezUuid()).LogFailure();
      return;
    }
  case ActionType::PickContextObject:
    //pDocument->SetContext();
    return;
  }
}


