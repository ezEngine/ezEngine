#include <PCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectContextAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezGameObjectContextActions::s_hCategory;
ezActionDescriptorHandle ezGameObjectContextActions::s_hPickContextScene;
ezActionDescriptorHandle ezGameObjectContextActions::s_hPickContextObject;
ezActionDescriptorHandle ezGameObjectContextActions::s_hClearContextObject;

void ezGameObjectContextActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("GameObjectContextCategory");
  s_hPickContextScene = EZ_REGISTER_ACTION_1("GameObjectContext.PickContextScene", ezActionScope::Window, "Game Object Context", "",
                                             ezGameObjectContextAction, ezGameObjectContextAction::ActionType::PickContextScene);
  s_hPickContextObject = EZ_REGISTER_ACTION_1("GameObjectContext.PickContextObject", ezActionScope::Window, "Game Object Context", "",
                                              ezGameObjectContextAction, ezGameObjectContextAction::ActionType::PickContextObject);
  s_hClearContextObject = EZ_REGISTER_ACTION_1("GameObjectContext.ClearContextObject", ezActionScope::Window, "Game Object Context", "",
                                               ezGameObjectContextAction, ezGameObjectContextAction::ActionType::ClearContextObject);
}


void ezGameObjectContextActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hPickContextScene);
  ezActionManager::UnregisterAction(s_hPickContextObject);
  ezActionManager::UnregisterAction(s_hClearContextObject);
}

void ezGameObjectContextActions::MapToolbarActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 10.0f);

  ezStringBuilder szSubPath(szPath, "/GameObjectContextCategory");
  pMap->MapAction(s_hPickContextScene, szSubPath, 1.0f);
}


void ezGameObjectContextActions::MapContextMenuActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 10.0f);

  ezStringBuilder szSubPath(szPath, "/GameObjectContextCategory");
  pMap->MapAction(s_hPickContextObject, szSubPath, 1.0f);
  pMap->MapAction(s_hClearContextObject, szSubPath, 2.0f);
}

ezGameObjectContextAction::ezGameObjectContextAction(const ezActionContext& context, const char* szName,
                                                     ezGameObjectContextAction::ActionType type)
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
    case ActionType::ClearContextObject:
      SetIconPath(":/EditorPluginAssets/PickTarget16.png");
      break;
  }

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(
      ezMakeDelegate(&ezGameObjectContextAction::SelectionEventHandler, this));
  Update();
}

ezGameObjectContextAction::~ezGameObjectContextAction()
{
  m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(
      ezMakeDelegate(&ezGameObjectContextAction::SelectionEventHandler, this));
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
    {
      const auto& selection = pDocument->GetSelectionManager()->GetSelection();
      if (selection.GetCount() == 1)
      {
        if (selection[0]->GetType() == ezGetStaticRTTI<ezGameObject>())
        {
          pDocument->SetContext(document, selection[0]->GetGuid()).LogFailure();
        }
      }
    }
      return;
    case ActionType::ClearContextObject:
    {
      pDocument->SetContext(document, ezUuid()).LogFailure();
    }
      return;
  }
}

void ezGameObjectContextAction::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  Update();
}

void ezGameObjectContextAction::Update()
{
  ezGameObjectContextDocument* pDocument = static_cast<ezGameObjectContextDocument*>(GetContext().m_pDocument);

  switch (m_Type)
  {
    case ActionType::PickContextObject:
    {
      const auto& selection = pDocument->GetSelectionManager()->GetSelection();
      bool bIsSingleGameObject = selection.GetCount() == 1 && selection[0]->GetType() == ezGetStaticRTTI<ezGameObject>();
      SetEnabled(bIsSingleGameObject);
    }
      return;
  }
}
