#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QFileDialog>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectionAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezSelectionActions::s_hGroupSelectedItems;
ezActionDescriptorHandle ezSelectionActions::s_hCreateEmptyChildObject;
ezActionDescriptorHandle ezSelectionActions::s_hCreateEmptyObjectAtPosition;
ezActionDescriptorHandle ezSelectionActions::s_hHideSelectedObjects;
ezActionDescriptorHandle ezSelectionActions::s_hHideUnselectedObjects;
ezActionDescriptorHandle ezSelectionActions::s_hShowHiddenObjects;
ezActionDescriptorHandle ezSelectionActions::s_hPrefabMenu;
ezActionDescriptorHandle ezSelectionActions::s_hCreatePrefab;
ezActionDescriptorHandle ezSelectionActions::s_hRevertPrefab;
ezActionDescriptorHandle ezSelectionActions::s_hUnlinkFromPrefab;
ezActionDescriptorHandle ezSelectionActions::s_hOpenPrefabDocument;
ezActionDescriptorHandle ezSelectionActions::s_hDuplicateSpecial;
ezActionDescriptorHandle ezSelectionActions::s_hDeltaTransform;
ezActionDescriptorHandle ezSelectionActions::s_hSnapObjectToCamera;
ezActionDescriptorHandle ezSelectionActions::s_hAttachToObject;
ezActionDescriptorHandle ezSelectionActions::s_hDetachFromParent;
ezActionDescriptorHandle ezSelectionActions::s_hConvertToEnginePrefab;
ezActionDescriptorHandle ezSelectionActions::s_hConvertToEditorPrefab;
ezActionDescriptorHandle ezSelectionActions::s_hCopyReference;



void ezSelectionActions::RegisterActions()
{
  s_hGroupSelectedItems = EZ_REGISTER_ACTION_1("Selection.GroupItems", ezActionScope::Document, "Scene - Selection", "Ctrl+G", ezSelectionAction,
    ezSelectionAction::ActionType::GroupSelectedItems);
  s_hCreateEmptyChildObject = EZ_REGISTER_ACTION_1("Selection.CreateEmptyChildObject", ezActionScope::Document, "Scene - Selection", "",
    ezSelectionAction, ezSelectionAction::ActionType::CreateEmptyChildObject);
  s_hCreateEmptyObjectAtPosition = EZ_REGISTER_ACTION_1("Selection.CreateEmptyObjectAtPosition", ezActionScope::Document, "Scene - Selection",
    "Ctrl+Shift+X", ezSelectionAction, ezSelectionAction::ActionType::CreateEmptyObjectAtPosition);
  s_hHideSelectedObjects = EZ_REGISTER_ACTION_1(
    "Selection.HideItems", ezActionScope::Document, "Scene - Selection", "H", ezSelectionAction, ezSelectionAction::ActionType::HideSelectedObjects);
  s_hHideUnselectedObjects = EZ_REGISTER_ACTION_1("Selection.HideUnselectedItems", ezActionScope::Document, "Scene - Selection", "Shift+H",
    ezSelectionAction, ezSelectionAction::ActionType::HideUnselectedObjects);
  s_hShowHiddenObjects = EZ_REGISTER_ACTION_1("Selection.ShowHidden", ezActionScope::Document, "Scene - Selection", "Ctrl+H", ezSelectionAction,
    ezSelectionAction::ActionType::ShowHiddenObjects);
  s_hAttachToObject = EZ_REGISTER_ACTION_1(
    "Selection.Attach", ezActionScope::Document, "Scene - Selection", "", ezSelectionAction, ezSelectionAction::ActionType::AttachToObject);
  s_hDetachFromParent = EZ_REGISTER_ACTION_1(
    "Selection.Detach", ezActionScope::Document, "Scene - Selection", "", ezSelectionAction, ezSelectionAction::ActionType::DetachFromParent);

  s_hPrefabMenu = EZ_REGISTER_MENU_WITH_ICON("Prefabs.Menu", ":/AssetIcons/Prefab.svg");
  s_hCreatePrefab =
    EZ_REGISTER_ACTION_1("Prefabs.Create", ezActionScope::Document, "Prefabs", "", ezSelectionAction, ezSelectionAction::ActionType::CreatePrefab);
  s_hRevertPrefab =
    EZ_REGISTER_ACTION_1("Prefabs.Revert", ezActionScope::Document, "Prefabs", "", ezSelectionAction, ezSelectionAction::ActionType::RevertPrefab);
  s_hUnlinkFromPrefab = EZ_REGISTER_ACTION_1(
    "Prefabs.Unlink", ezActionScope::Document, "Prefabs", "", ezSelectionAction, ezSelectionAction::ActionType::UnlinkFromPrefab);
  s_hOpenPrefabDocument = EZ_REGISTER_ACTION_1(
    "Prefabs.OpenDocument", ezActionScope::Document, "Prefabs", "", ezSelectionAction, ezSelectionAction::ActionType::OpenPrefabDocument);
  s_hConvertToEnginePrefab = EZ_REGISTER_ACTION_1(
    "Prefabs.ConvertToEngine", ezActionScope::Document, "Prefabs", "", ezSelectionAction, ezSelectionAction::ActionType::ConvertToEnginePrefab);
  s_hConvertToEditorPrefab = EZ_REGISTER_ACTION_1(
    "Prefabs.ConvertToEditor", ezActionScope::Document, "Prefabs", "", ezSelectionAction, ezSelectionAction::ActionType::ConvertToEditorPrefab);

  s_hDuplicateSpecial = EZ_REGISTER_ACTION_1("Selection.DuplicateSpecial", ezActionScope::Document, "Scene - Selection", "Ctrl+D", ezSelectionAction,
    ezSelectionAction::ActionType::DuplicateSpecial);
  s_hDeltaTransform = EZ_REGISTER_ACTION_1("Selection.DeltaTransform", ezActionScope::Document, "Scene - Selection", "Ctrl+M", ezSelectionAction,
    ezSelectionAction::ActionType::DeltaTransform);
  s_hSnapObjectToCamera = EZ_REGISTER_ACTION_1(
    "Scene.Camera.SnapObjectToCamera", ezActionScope::Document, "Camera", "", ezSelectionAction, ezSelectionAction::ActionType::SnapObjectToCamera);
  s_hCopyReference = EZ_REGISTER_ACTION_1(
    "Selection.CopyReference", ezActionScope::Document, "Scene - Selection", "", ezSelectionAction, ezSelectionAction::ActionType::CopyReference);
}

void ezSelectionActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hGroupSelectedItems);
  ezActionManager::UnregisterAction(s_hCreateEmptyChildObject);
  ezActionManager::UnregisterAction(s_hCreateEmptyObjectAtPosition);
  ezActionManager::UnregisterAction(s_hHideSelectedObjects);
  ezActionManager::UnregisterAction(s_hHideUnselectedObjects);
  ezActionManager::UnregisterAction(s_hShowHiddenObjects);
  ezActionManager::UnregisterAction(s_hPrefabMenu);
  ezActionManager::UnregisterAction(s_hCreatePrefab);
  ezActionManager::UnregisterAction(s_hRevertPrefab);
  ezActionManager::UnregisterAction(s_hUnlinkFromPrefab);
  ezActionManager::UnregisterAction(s_hOpenPrefabDocument);
  ezActionManager::UnregisterAction(s_hDuplicateSpecial);
  ezActionManager::UnregisterAction(s_hDeltaTransform);
  ezActionManager::UnregisterAction(s_hSnapObjectToCamera);
  ezActionManager::UnregisterAction(s_hAttachToObject);
  ezActionManager::UnregisterAction(s_hDetachFromParent);
  ezActionManager::UnregisterAction(s_hConvertToEditorPrefab);
  ezActionManager::UnregisterAction(s_hConvertToEnginePrefab);
  ezActionManager::UnregisterAction(s_hCopyReference);
}

void ezSelectionActions::MapActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCreateEmptyChildObject, "G.Selection", 1.0f);
  pMap->MapAction(s_hCreateEmptyObjectAtPosition, "G.Selection", 1.1f);
  pMap->MapAction(s_hGroupSelectedItems, "G.Selection", 3.7f);
  pMap->MapAction(s_hHideSelectedObjects, "G.Selection", 4.0f);
  pMap->MapAction(s_hHideUnselectedObjects, "G.Selection", 5.0f);
  pMap->MapAction(s_hShowHiddenObjects, "G.Selection", 6.0f);
  pMap->MapAction(s_hDuplicateSpecial, "G.Selection", 7.0f);
  pMap->MapAction(s_hDeltaTransform, "G.Selection", 7.1f);
  pMap->MapAction(s_hAttachToObject, "G.Selection", 7.2f);
  pMap->MapAction(s_hDetachFromParent, "G.Selection", 7.3f);
  pMap->MapAction(s_hSnapObjectToCamera, "G.Selection", 9.0f);
  pMap->MapAction(s_hCopyReference, "G.Selection", 10.0f);

  MapPrefabActions(sMapping, 0.0f);
}

void ezSelectionActions::MapPrefabActions(ezStringView sMapping, float fPriority)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hPrefabMenu, "G.Selection", fPriority);

  pMap->MapAction(s_hOpenPrefabDocument, "G.Selection", "Prefabs.Menu", 1.0f);
  pMap->MapAction(s_hRevertPrefab, "G.Selection", "Prefabs.Menu", 2.0f);
  pMap->MapAction(s_hCreatePrefab, "G.Selection", "Prefabs.Menu", 3.0f);
  pMap->MapAction(s_hUnlinkFromPrefab, "G.Selection", "Prefabs.Menu", 4.0f);
  pMap->MapAction(s_hConvertToEditorPrefab, "G.Selection", "Prefabs.Menu", 5.0f);
  pMap->MapAction(s_hConvertToEnginePrefab, "G.Selection", "Prefabs.Menu", 6.0f);
}

void ezSelectionActions::MapContextMenuActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCreateEmptyChildObject, "G.Selection", 0.5f);
  pMap->MapAction(s_hGroupSelectedItems, "G.Selection", 2.0f);
  pMap->MapAction(s_hHideSelectedObjects, "G.Selection", 3.0f);
  pMap->MapAction(s_hDetachFromParent, "G.Selection", 3.2f);
  pMap->MapAction(s_hCopyReference, "G.Selection", 4.0f);

  MapPrefabActions(sMapping, 4.0f);
}


void ezSelectionActions::MapViewContextMenuActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hGroupSelectedItems, "G.Selection", 2.0f);
  pMap->MapAction(s_hHideSelectedObjects, "G.Selection", 3.0f);
  pMap->MapAction(s_hAttachToObject, "G.Selection", 3.1f);
  pMap->MapAction(s_hDetachFromParent, "G.Selection", 3.2f);
  pMap->MapAction(s_hSnapObjectToCamera, "G.Selection", 5.0f);
  pMap->MapAction(s_hCopyReference, "G.Selection", 6.0f);

  MapPrefabActions(sMapping, 7.0f);
}

ezSelectionAction::ezSelectionAction(const ezActionContext& context, const char* szName, ezSelectionAction::ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pSceneDocument = const_cast<ezSceneDocument*>(static_cast<const ezSceneDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::GroupSelectedItems:
      SetIconPath(":/EditorPluginScene/Icons/GroupSelection.svg");
      break;
    case ActionType::CreateEmptyChildObject:
      SetIconPath(":/EditorPluginScene/Icons/CreateNode.svg");
      break;
    case ActionType::CreateEmptyObjectAtPosition:
      SetIconPath(":/EditorPluginScene/Icons/CreateNode.svg");
      break;
    case ActionType::HideSelectedObjects:
      SetIconPath(":/EditorPluginScene/Icons/HideSelected.svg");
      break;
    case ActionType::HideUnselectedObjects:
      SetIconPath(":/EditorPluginScene/Icons/HideUnselected.svg");
      break;
    case ActionType::ShowHiddenObjects:
      SetIconPath(":/EditorPluginScene/Icons/ShowHidden.svg");
      break;
    case ActionType::CreatePrefab:
      SetIconPath(":/EditorPluginScene/Icons/PrefabCreate.svg");
      break;
    case ActionType::RevertPrefab:
      SetIconPath(":/EditorPluginScene/Icons/PrefabRevert.svg");
      break;
    case ActionType::UnlinkFromPrefab:
      SetIconPath(":/EditorPluginScene/Icons/PrefabUnlink.svg");
      break;
    case ActionType::OpenPrefabDocument:
      SetIconPath(":/EditorPluginScene/Icons/PrefabOpenDocument.svg");
      break;
    case ActionType::DuplicateSpecial:
      SetIconPath(":/EditorPluginScene/Icons/Duplicate.svg");
      break;
    case ActionType::DeltaTransform:
      // SetIconPath(":/EditorPluginScene/Icons/Duplicate.svg"); // TODO Icon
      break;
    case ActionType::SnapObjectToCamera:
      // SetIconPath(":/EditorPluginScene/Icons/Duplicate.svg"); // TODO Icon
      break;
    case ActionType::AttachToObject:
      // SetIconPath(":/EditorPluginScene/Icons/Duplicate.svg"); // TODO Icon
      break;
    case ActionType::DetachFromParent:
      // SetIconPath(":/EditorPluginScene/Icons/Duplicate.svg"); // TODO Icon
      break;
    case ActionType::ConvertToEditorPrefab:
      // SetIconPath(":/EditorPluginScene/PrefabRevert.png"); // TODO Icon
      break;
    case ActionType::ConvertToEnginePrefab:
      // SetIconPath(":/EditorPluginScene/PrefabRevert.png"); // TODO Icon
      break;
    case ActionType::CopyReference:
      // SetIconPath(":/EditorPluginScene/PrefabRevert.png"); // TODO Icon
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
    case ActionType::GroupSelectedItems:
      m_pSceneDocument->GroupSelection();
      return;
    case ActionType::CreateEmptyChildObject:
    {
      auto res = m_pSceneDocument->CreateEmptyObject(true, false);
      ezQtUiServices::MessageBoxStatus(res, "Object creation failed.");
      return;
    }
    case ActionType::CreateEmptyObjectAtPosition:
    {
      auto res = m_pSceneDocument->CreateEmptyObject(false, true);
      ezQtUiServices::MessageBoxStatus(res, "Object creation failed.");
      return;
    }
    case ActionType::HideSelectedObjects:
      m_pSceneDocument->ShowOrHideSelectedObjects(ezSceneDocument::ShowOrHide::Hide);
      m_pSceneDocument->ShowDocumentStatus("Hiding selected objects");
      break;
    case ActionType::HideUnselectedObjects:
      m_pSceneDocument->HideUnselectedObjects();
      m_pSceneDocument->ShowDocumentStatus("Hiding unselected objects");
      break;
    case ActionType::ShowHiddenObjects:
      m_pSceneDocument->ShowOrHideAllObjects(ezSceneDocument::ShowOrHide::Show);
      m_pSceneDocument->ShowDocumentStatus("Showing hidden objects");
      break;
    case ActionType::CreatePrefab:
      CreatePrefab();
      break;

    case ActionType::RevertPrefab:
    {
      if (ezQtUiServices::MessageBoxQuestion("Discard all modifications to the selected prefabs and revert to the prefab template state?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
      {
        ezHybridArray<ezSelectionEntry, 64> selection;
        m_pSceneDocument->GetSelectionManager()->GetTopLevelSelectionOfType(ezGetStaticRTTI<ezGameObject>(), selection);

        ezHybridArray<const ezDocumentObject*, 64> selection2;
        selection2.SetCount(selection.GetCount());
        for (ezUInt32 i = 0; i < selection.GetCount(); ++i)
        {
          selection2[i] = selection[i].m_pObject;
        }

        m_pSceneDocument->RevertPrefabs(selection2);
      }
    }
    break;

    case ActionType::UnlinkFromPrefab:
    {
      if (ezQtUiServices::MessageBoxQuestion("Unlink the selected prefab instances from their templates?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
      {
        ezHybridArray<ezSelectionEntry, 64> selection;
        m_pSceneDocument->GetSelectionManager()->GetTopLevelSelectionOfType(ezGetStaticRTTI<ezGameObject>(), selection);

        ezHybridArray<const ezDocumentObject*, 64> selection2;
        selection2.SetCount(selection.GetCount());
        for (ezUInt32 i = 0; i < selection.GetCount(); ++i)
        {
          selection2[i] = selection[i].m_pObject;
        }

        m_pSceneDocument->UnlinkPrefabs(selection2);
      }
    }
    break;

    case ActionType::OpenPrefabDocument:
      OpenPrefabDocument();
      break;

    case ActionType::DuplicateSpecial:
      m_pSceneDocument->DuplicateSpecial();
      break;

    case ActionType::DeltaTransform:
      m_pSceneDocument->DeltaTransform();
      break;

    case ActionType::SnapObjectToCamera:
      m_pSceneDocument->SnapObjectToCamera();
      break;

    case ActionType::AttachToObject:
      m_pSceneDocument->AttachToObject();
      break;
    case ActionType::DetachFromParent:
      m_pSceneDocument->DetachFromParent();
      break;

    case ActionType::CopyReference:
      m_pSceneDocument->CopyReference();
      break;

    case ActionType::ConvertToEditorPrefab:
    {
      ezHybridArray<ezSelectionEntry, 64> selection;
      m_pSceneDocument->GetSelectionManager()->GetTopLevelSelectionOfType(ezGetStaticRTTI<ezGameObject>(), selection);

      ezHybridArray<const ezDocumentObject*, 64> selection2;
      selection2.SetCount(selection.GetCount());
      for (ezUInt32 i = 0; i < selection.GetCount(); ++i)
      {
        selection2[i] = selection[i].m_pObject;
      }

      m_pSceneDocument->ConvertToEditorPrefab(selection2);
    }
    break;

    case ActionType::ConvertToEnginePrefab:
    {
      if (ezQtUiServices::MessageBoxQuestion("Discard all modifications to the selected prefabs and convert them to engine prefabs?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
      {
        ezHybridArray<ezSelectionEntry, 64> selection;
        m_pSceneDocument->GetSelectionManager()->GetTopLevelSelectionOfType(ezGetStaticRTTI<ezGameObject>(), selection);

        ezHybridArray<const ezDocumentObject*, 64> selection2;
        selection2.SetCount(selection.GetCount());
        for (ezUInt32 i = 0; i < selection.GetCount(); ++i)
        {
          selection2[i] = selection[i].m_pObject;
        }

        m_pSceneDocument->ConvertToEnginePrefab(selection2);
      }
    }
    break;
  }
}


void ezSelectionAction::OpenPrefabDocument()
{
  const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

  if (sel.GetCount() != 1)
    return;

  const ezSceneDocument* pScene = static_cast<const ezSceneDocument*>(m_Context.m_pDocument);


  ezUuid PrefabAsset;
  if (pScene->IsObjectEnginePrefab(sel[0]->GetGuid(), &PrefabAsset))
  {
    // PrefabAsset is all we need
  }
  else
  {
    auto pMeta = pScene->m_DocumentObjectMetaData->BeginReadMetaData(sel[0]->GetGuid());
    PrefabAsset = pMeta->m_CreateFromPrefab;
    pScene->m_DocumentObjectMetaData->EndReadMetaData();
  }

  auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(PrefabAsset);
  if (pAsset)
  {
    ezQtEditorApp::GetSingleton()->OpenDocumentQueued(pAsset->m_pAssetInfo->m_Path.GetAbsolutePath());
  }
  else
  {
    ezQtUiServices::MessageBoxWarning("The prefab asset of this instance is currently unknown. It may have been deleted. Try updating the "
                                      "asset library ('Check FileSystem'), if it should be there.");
  }
}

void ezSelectionAction::CreatePrefab()
{
  static ezString sSearchDir = ezToolsProject::GetSingleton()->GetProjectFile();

  ezStringBuilder sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Prefab"),
    QString::fromUtf8(sSearchDir.GetData()), QString::fromUtf8("*.ezPrefab"), nullptr, QFileDialog::Option::DontResolveSymlinks)
                            .toUtf8()
                            .data();

  if (!sFile.IsEmpty())
  {
    sFile.ChangeFileExtension("ezPrefab");

    sSearchDir = sFile.GetFileDirectory();

    if (ezOSFile::ExistsFile(sFile))
    {
      ezQtUiServices::MessageBoxInformation("You currently cannot replace an existing prefab this way. Please choose a new prefab file.");
      return;
    }

    auto res = m_pSceneDocument->CreatePrefabDocumentFromSelection(sFile, ezGetStaticRTTI<ezGameObject>());
    m_pSceneDocument->ScheduleSendObjectSelection(); // fix selection of prefab object
    ezQtUiServices::MessageBoxStatus(res, "Failed to create Prefab", "Successfully created Prefab");
  }
}

void ezSelectionAction::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  UpdateEnableState();
}

void ezSelectionAction::UpdateEnableState()
{
  if (m_Type == ActionType::HideSelectedObjects || m_Type == ActionType::DuplicateSpecial || m_Type == ActionType::DeltaTransform ||
      m_Type == ActionType::SnapObjectToCamera || m_Type == ActionType::DetachFromParent || m_Type == ActionType::HideUnselectedObjects ||
      m_Type == ActionType::AttachToObject)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }

  if (m_Type == ActionType::GroupSelectedItems)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() > 1);
  }

  if (m_Type == ActionType::CreateEmptyChildObject)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() <= 1);
  }

  if (m_Type == ActionType::CopyReference)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() == 1);
  }

  if (m_Type == ActionType::OpenPrefabDocument)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.GetCount() != 1)
    {
      SetEnabled(false);
      return;
    }

    const ezSceneDocument* pScene = static_cast<const ezSceneDocument*>(m_Context.m_pDocument);
    const bool bIsPrefab = pScene->IsObjectEditorPrefab(sel[0]->GetGuid()) || pScene->IsObjectEnginePrefab(sel[0]->GetGuid());

    SetEnabled(bIsPrefab);
    return;
  }

  if (m_Type == ActionType::RevertPrefab || m_Type == ActionType::UnlinkFromPrefab || m_Type == ActionType::ConvertToEnginePrefab ||
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
      SetEnabled(true);
      return;
    }

    const bool bShouldBePrefab =
      (m_Type == ActionType::RevertPrefab) || (m_Type == ActionType::ConvertToEnginePrefab) || (m_Type == ActionType::UnlinkFromPrefab);

    const ezSceneDocument* pScene = static_cast<const ezSceneDocument*>(m_Context.m_pDocument);
    const bool bIsPrefab = pScene->IsObjectEditorPrefab(sel[0]->GetGuid());

    SetEnabled(bIsPrefab == bShouldBePrefab);
  }

  if (m_Type == ActionType::ConvertToEditorPrefab)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.IsEmpty())
    {
      SetEnabled(false);
      return;
    }

    const ezSceneDocument* pScene = static_cast<const ezSceneDocument*>(m_Context.m_pDocument);
    const bool bIsPrefab = pScene->IsObjectEnginePrefab(sel[0]->GetGuid());

    SetEnabled(bIsPrefab);
  }
}
