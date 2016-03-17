#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <QFileDialog>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Foundation/IO/OSFile.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Core/World/GameObject.h>
#include <ToolsFoundation/Command/TreeCommands.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectionAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezActionDescriptorHandle ezSelectionActions::s_hSelectionCategory;
ezActionDescriptorHandle ezSelectionActions::s_hShowInScenegraph;
ezActionDescriptorHandle ezSelectionActions::s_hFocusOnSelection;
ezActionDescriptorHandle ezSelectionActions::s_hFocusOnSelectionAllViews;
ezActionDescriptorHandle ezSelectionActions::s_hGroupSelectedItems;
ezActionDescriptorHandle ezSelectionActions::s_hCreateEmptyNode;
ezActionDescriptorHandle ezSelectionActions::s_hHideSelectedObjects;
ezActionDescriptorHandle ezSelectionActions::s_hHideUnselectedObjects;
ezActionDescriptorHandle ezSelectionActions::s_hShowHiddenObjects;
ezActionDescriptorHandle ezSelectionActions::s_hPrefabMenu;
ezActionDescriptorHandle ezSelectionActions::s_hCreatePrefab;
ezActionDescriptorHandle ezSelectionActions::s_hRevertPrefab;
ezActionDescriptorHandle ezSelectionActions::s_hUnlinkFromPrefab;
ezActionDescriptorHandle ezSelectionActions::s_hOpenPrefabDocument;
ezActionDescriptorHandle ezSelectionActions::s_hDuplicateSpecial;


void ezSelectionActions::RegisterActions()
{
  s_hSelectionCategory = EZ_REGISTER_CATEGORY("SelectionCategory");
  s_hShowInScenegraph = EZ_REGISTER_ACTION_1("Selection.ShowInScenegraph", ezActionScope::Document, "Scene - Selection", "Ctrl+T", ezSelectionAction, ezSelectionAction::ActionType::ShowInScenegraph);
  s_hFocusOnSelection = EZ_REGISTER_ACTION_1("Selection.FocusSingleView", ezActionScope::Document, "Scene - Selection", "F", ezSelectionAction, ezSelectionAction::ActionType::FocusOnSelection);
  s_hFocusOnSelectionAllViews = EZ_REGISTER_ACTION_1("Selection.FocusAllViews", ezActionScope::Document, "Scene - Selection", "Ctrl+K,Ctrl+F", ezSelectionAction, ezSelectionAction::ActionType::FocusOnSelectionAllViews);
  s_hGroupSelectedItems = EZ_REGISTER_ACTION_1("Selection.GroupItems", ezActionScope::Document, "Scene - Selection", "G", ezSelectionAction, ezSelectionAction::ActionType::GroupSelectedItems);
  s_hCreateEmptyNode = EZ_REGISTER_ACTION_1("Selection.CreateEmptyNode", ezActionScope::Document, "Scene - Selection", "Ctrl+Shift+N", ezSelectionAction, ezSelectionAction::ActionType::CreateEmptyNode);
  s_hHideSelectedObjects = EZ_REGISTER_ACTION_1("Selection.HideItems", ezActionScope::Document, "Scene - Selection", "H", ezSelectionAction, ezSelectionAction::ActionType::HideSelectedObjects);
  s_hHideUnselectedObjects = EZ_REGISTER_ACTION_1("Selection.HideUnselectedItems", ezActionScope::Document, "Scene - Selection", "Shift+H", ezSelectionAction, ezSelectionAction::ActionType::HideUnselectedObjects);
  s_hShowHiddenObjects = EZ_REGISTER_ACTION_1("Selection.ShowHidden", ezActionScope::Document, "Scene - Selection", "Ctrl+H", ezSelectionAction, ezSelectionAction::ActionType::ShowHiddenObjects);

  s_hPrefabMenu = EZ_REGISTER_MENU_WITH_ICON("Prefabs.Menu", ":/AssetIcons/Prefab.png");
  s_hCreatePrefab = EZ_REGISTER_ACTION_1("Prefabs.Create", ezActionScope::Document, "Prefabs", "Ctrl+P,Ctrl+C", ezSelectionAction, ezSelectionAction::ActionType::CreatePrefab);
  s_hRevertPrefab = EZ_REGISTER_ACTION_1("Prefabs.Revert", ezActionScope::Document, "Prefabs", "Ctrl+P,Ctrl+R", ezSelectionAction, ezSelectionAction::ActionType::RevertPrefab);
  s_hUnlinkFromPrefab = EZ_REGISTER_ACTION_1("Prefabs.Unlink", ezActionScope::Document, "Prefabs", "Ctrl+P,Ctrl+U", ezSelectionAction, ezSelectionAction::ActionType::UnlinkFromPrefab);
  s_hOpenPrefabDocument = EZ_REGISTER_ACTION_1("Prefabs.OpenDocument", ezActionScope::Document, "Prefabs", "Ctrl+P,Ctrl+O", ezSelectionAction, ezSelectionAction::ActionType::OpenPrefabDocument);

  s_hDuplicateSpecial = EZ_REGISTER_ACTION_1("Selection.DuplicateSpecial", ezActionScope::Document, "Scene - Selection", "Ctrl+D", ezSelectionAction, ezSelectionAction::ActionType::DuplicateSpecial);
}

void ezSelectionActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSelectionCategory);
  ezActionManager::UnregisterAction(s_hShowInScenegraph);
  ezActionManager::UnregisterAction(s_hFocusOnSelection);
  ezActionManager::UnregisterAction(s_hFocusOnSelectionAllViews);
  ezActionManager::UnregisterAction(s_hGroupSelectedItems);
  ezActionManager::UnregisterAction(s_hCreateEmptyNode);
  ezActionManager::UnregisterAction(s_hHideSelectedObjects);
  ezActionManager::UnregisterAction(s_hHideUnselectedObjects);
  ezActionManager::UnregisterAction(s_hShowHiddenObjects);
  ezActionManager::UnregisterAction(s_hPrefabMenu);
  ezActionManager::UnregisterAction(s_hCreatePrefab);
  ezActionManager::UnregisterAction(s_hRevertPrefab);
  ezActionManager::UnregisterAction(s_hUnlinkFromPrefab);
  ezActionManager::UnregisterAction(s_hOpenPrefabDocument);
  ezActionManager::UnregisterAction(s_hDuplicateSpecial);
}

void ezSelectionActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/SelectionCategory");

  pMap->MapAction(s_hSelectionCategory, szPath, 5.0f);
  
  pMap->MapAction(s_hCreateEmptyNode, sSubPath, 1.0f);
  pMap->MapAction(s_hShowInScenegraph, sSubPath, 2.0f);
  pMap->MapAction(s_hFocusOnSelection, sSubPath, 3.0f);
  pMap->MapAction(s_hFocusOnSelectionAllViews, sSubPath, 3.5f);
  pMap->MapAction(s_hGroupSelectedItems, sSubPath, 3.7f);
  pMap->MapAction(s_hHideSelectedObjects, sSubPath, 4.0f);
  pMap->MapAction(s_hHideUnselectedObjects, sSubPath, 5.0f);
  pMap->MapAction(s_hShowHiddenObjects, sSubPath, 6.0f);
  pMap->MapAction(s_hDuplicateSpecial, sSubPath, 7.0f);

  MapPrefabActions(szMapping, sSubPath, 1.0f);
}

void ezSelectionActions::MapPrefabActions(const char* szMapping, const char* szPath, float fPriority)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sPrefabSubPath(szPath, "/Prefabs.Menu");
  pMap->MapAction(s_hPrefabMenu, szPath, fPriority);

  pMap->MapAction(s_hOpenPrefabDocument, sPrefabSubPath, 1.0f);
  pMap->MapAction(s_hRevertPrefab, sPrefabSubPath, 2.0f);
  pMap->MapAction(s_hCreatePrefab, sPrefabSubPath, 3.0f);
  pMap->MapAction(s_hUnlinkFromPrefab, sPrefabSubPath, 4.0f);
}

void ezSelectionActions::MapContextMenuActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/SelectionCategory");

  pMap->MapAction(s_hSelectionCategory, szPath, 5.0f);
  
  pMap->MapAction(s_hCreateEmptyNode, sSubPath, 0.5f);
  pMap->MapAction(s_hFocusOnSelectionAllViews, sSubPath, 1.0f);
  pMap->MapAction(s_hGroupSelectedItems, sSubPath, 2.0f);
  pMap->MapAction(s_hHideSelectedObjects, sSubPath, 3.0f);

  MapPrefabActions(szMapping, sSubPath, 4.0f);
}

ezSelectionAction::ezSelectionAction(const ezActionContext& context, const char* szName, ezSelectionAction::ActionType type) : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_pSceneDocument = static_cast<ezSceneDocument*>(context.m_pDocument);

  switch (m_Type)
  {
  case ActionType::ShowInScenegraph:
    SetIconPath(":/EditorPluginScene/Icons/Scenegraph16.png");
    break;
  case ActionType::FocusOnSelection:
    SetIconPath(":/EditorPluginScene/Icons/FocusOnSelection16.png");
    break;
  case ActionType::FocusOnSelectionAllViews:
    SetIconPath(":/EditorPluginScene/Icons/FocusOnSelectionAllViews16.png");
    break;
  case ActionType::GroupSelectedItems:
    SetIconPath(":/EditorPluginScene/Icons/GroupSelection16.png");
    break;
  case ActionType::CreateEmptyNode:
    SetIconPath(":/EditorPluginScene/Icons/CreateNode16.png");
    break;
  case ActionType::HideSelectedObjects:
    SetIconPath(":/EditorPluginScene/Icons/HideSelected16.png");
    break;
  case ActionType::HideUnselectedObjects:
    SetIconPath(":/EditorPluginScene/Icons/HideUnselected16.png");
    break;
  case ActionType::ShowHiddenObjects:
    SetIconPath(":/EditorPluginScene/Icons/ShowHidden16.png");
    break;
  case ActionType::CreatePrefab:
    SetIconPath(":/EditorPluginScene/PrefabCreate.png");
    break;
  case ActionType::RevertPrefab:
    SetIconPath(":/EditorPluginScene/PrefabRevert.png");
    break;
  case ActionType::UnlinkFromPrefab:
    SetIconPath(":/EditorPluginScene/PrefabUnlink.png");
    break;
  case ActionType::OpenPrefabDocument:
    SetIconPath(":/EditorPluginScene/PrefabOpenDocument.png");
    break;
  case ActionType::DuplicateSpecial:
    SetIconPath(":/EditorPluginScene/Icons/Duplicate16.png");
    break;
  }

  UpdateEnableState();

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezSelectionAction::SelectionEventHandler, this));
}


ezSelectionAction::~ezSelectionAction()
{
  m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezSelectionAction::SelectionEventHandler, this));
}

void ezSelectionAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
  case ActionType::ShowInScenegraph:
    m_pSceneDocument->TriggerShowSelectionInScenegraph();
    return;
  case ActionType::FocusOnSelection:
    m_pSceneDocument->TriggerFocusOnSelection(false);
    return;
  case ActionType::FocusOnSelectionAllViews:
    m_pSceneDocument->TriggerFocusOnSelection(true);
    return;
  case ActionType::GroupSelectedItems:
    m_pSceneDocument->GroupSelection();
    return;
  case ActionType::CreateEmptyNode:
    m_pSceneDocument->CreateEmptyNode();
    return;
  case ActionType::HideSelectedObjects:
    m_pSceneDocument->ShowOrHideSelectedObjects(ezSceneDocument::ShowOrHide::Hide);
    break;
  case ActionType::HideUnselectedObjects:
    m_pSceneDocument->HideUnselectedObjects();
    break;
  case ActionType::ShowHiddenObjects:
    m_pSceneDocument->ShowOrHideAllObjects(ezSceneDocument::ShowOrHide::Show);
    break;
  case ActionType::CreatePrefab:
    CreatePrefab();
    break;

  case ActionType::RevertPrefab:
    {
      if (ezUIServices::MessageBoxQuestion("Discard all modifications to the selected prefabs and revert to the prefab template state?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
      {
        const ezDeque<const ezDocumentObject*> sel = m_pSceneDocument->GetSelectionManager()->GetTopLevelSelection(ezGetStaticRTTI<ezGameObject>());
        m_pSceneDocument->RevertPrefabs(sel);
      }
    }
    break;

  case ActionType::UnlinkFromPrefab:
    {
      if (ezUIServices::MessageBoxQuestion("Unlink the selected prefab instances from their templates?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
      {
        const ezDeque<const ezDocumentObject*> sel = m_pSceneDocument->GetSelectionManager()->GetTopLevelSelection(ezGetStaticRTTI<ezGameObject>());
        m_pSceneDocument->UnlinkPrefabs(sel);
      }
    }
    break;

  case ActionType::OpenPrefabDocument:
    OpenPrefabDocument();
    break;

  case ActionType::DuplicateSpecial:
    m_pSceneDocument->DuplicateSpecial();
    break;
  }
}


void ezSelectionAction::OpenPrefabDocument()
{
  const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

  if (sel.GetCount() != 1)
    return;

  ezSceneDocument* pScene = static_cast<ezSceneDocument*>(m_Context.m_pDocument);

  auto pMeta = pScene->m_ObjectMetaData.BeginReadMetaData(sel[0]->GetGuid());
  const ezUuid PrefabAsset = pMeta->m_CreateFromPrefab;
  pScene->m_ObjectMetaData.EndReadMetaData();

  auto pAsset = ezAssetCurator::GetSingleton()->GetAssetInfo(PrefabAsset);
  if (pAsset)
  {
    ezQtEditorApp::GetSingleton()->OpenDocument(pAsset->m_sAbsolutePath);
  }
  else
  {
    ezUIServices::MessageBoxWarning("The prefab asset of this instance is currently unknown. It may have been deleted. Try updating the asset library ('Check FileSystem'), if it should be there.");
  }
}

void ezSelectionAction::CreatePrefab()
{
  static ezString sSearchDir = ezToolsProject::GetSingleton()->GetProjectFile();

  ezStringBuilder sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Prefab"), QString::fromUtf8(sSearchDir.GetData()), QString::fromUtf8("*.ezPrefab")).toUtf8().data();

  if (!sFile.IsEmpty())
  {
    sFile.ChangeFileExtension("ezPrefab");

    sSearchDir = sFile.GetFileDirectory();

    if (ezOSFile::ExistsFile(sFile))
    {
      ezUIServices::MessageBoxInformation("You currently cannot replace an existing prefab this way. Please choose a new prefab file.");
      return;
    }

    auto res = m_pSceneDocument->CreatePrefabDocumentFromSelection(sFile);
    ezUIServices::MessageBoxStatus(res, "Failed to create Prefab", "Successfully created Prefab");
  }
}

void ezSelectionAction::SelectionEventHandler(const ezSelectionManager::Event& e)
{
  UpdateEnableState();

}

void ezSelectionAction::UpdateEnableState()
{
  if (m_Type == ActionType::FocusOnSelection ||
      m_Type == ActionType::FocusOnSelectionAllViews ||
      m_Type == ActionType::HideSelectedObjects ||
      m_Type == ActionType::ShowInScenegraph ||
      m_Type == ActionType::DuplicateSpecial ||
      m_Type == ActionType::HideUnselectedObjects)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }

  if (m_Type == ActionType::GroupSelectedItems)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() > 1);
  }

  if (m_Type == ActionType::CreateEmptyNode)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() <= 1);
  }

  if (m_Type == ActionType::OpenPrefabDocument)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.GetCount() != 1)
    {
      SetEnabled(false);
      return;
    }

    
  }

  if (m_Type == ActionType::RevertPrefab ||
      m_Type == ActionType::UnlinkFromPrefab ||
      m_Type == ActionType::OpenPrefabDocument ||
      m_Type == ActionType::CreatePrefab)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.IsEmpty())
    {
      SetEnabled(false);
      return;
    }

    if (m_Type == ActionType::CreatePrefab)
    {
      SetEnabled(sel.GetCount() == 1);
      return;
    }

    if (m_Type == ActionType::OpenPrefabDocument && (sel.GetCount() != 1))
    {
      SetEnabled(false);
      return;
    }

    const bool bShouldBePrefab = (m_Type == ActionType::RevertPrefab) || 
                                 (m_Type == ActionType::OpenPrefabDocument) || 
                                 (m_Type == ActionType::UnlinkFromPrefab);

    bool bIsPrefab = false;
    {
      ezSceneDocument* pScene = static_cast<ezSceneDocument*>(m_Context.m_pDocument);

      auto pMeta = pScene->m_ObjectMetaData.BeginReadMetaData(sel[0]->GetGuid());
      bIsPrefab = pMeta->m_CreateFromPrefab.IsValid();
      pScene->m_ObjectMetaData.EndReadMetaData();
    }

    SetEnabled(bIsPrefab == bShouldBePrefab);
  }
}

