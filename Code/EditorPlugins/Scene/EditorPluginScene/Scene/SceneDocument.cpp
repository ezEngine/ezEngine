#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorFramework/Preferences/QuadViewPreferences.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Dialogs/DeltaTransformDlg.moc.h>
#include <EditorPluginScene/Dialogs/DuplicateDlg.moc.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>
#include <QClipboard>
#include <QMenu>
#include <RendererCore/Components/CameraComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectDirectAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocument, 7, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezSceneDocument_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  static const ezRTTI* pRtti = ezRTTI::FindTypeByName("ezGameObject");

  if (e.m_pObject->GetDocumentObjectManager()->GetDocument()->GetDocumentTypeName() != "Prefab")
    return;

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  auto pParent = e.m_pObject->GetParent();
  if (pParent != nullptr)
  {
    if (pParent->GetTypeAccessor().GetType() == pRtti)
      return;
  }

  const ezString name = e.m_pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();
  if (name != "<Prefab-Root>")
    return;

  auto& props = *e.m_pPropertyStates;
  props["Name"].m_sNewLabelText = "Prefab.NameLabel";
  props["Active"].m_Visibility = ezPropertyUiState::Invisible;
  props["LocalPosition"].m_Visibility = ezPropertyUiState::Invisible;
  props["LocalRotation"].m_Visibility = ezPropertyUiState::Invisible;
  props["LocalScaling"].m_Visibility = ezPropertyUiState::Invisible;
  props["LocalUniformScaling"].m_Visibility = ezPropertyUiState::Invisible;
  props["GlobalKey"].m_Visibility = ezPropertyUiState::Invisible;
  props["Tags"].m_Visibility = ezPropertyUiState::Invisible;
}

ezSceneDocument::ezSceneDocument(ezStringView sDocumentPath, DocumentType documentType)
  : ezGameObjectDocument(sDocumentPath, EZ_DEFAULT_NEW(ezSceneObjectManager))
{
  m_DocumentType = documentType;
  m_GameMode = GameMode::Off;
  SetAddAmbientLight(IsPrefab());

  m_GameModeData[GameMode::Off].m_bRenderSelectionOverlay = true;
  m_GameModeData[GameMode::Off].m_bRenderShapeIcons = true;
  m_GameModeData[GameMode::Off].m_bRenderVisualizers = true;

  m_GameModeData[GameMode::Simulate].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Simulate].m_bRenderShapeIcons = false;
  m_GameModeData[GameMode::Simulate].m_bRenderVisualizers = false;

  m_GameModeData[GameMode::Play].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Play].m_bRenderShapeIcons = false;
  m_GameModeData[GameMode::Play].m_bRenderVisualizers = false;

  GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocument::SelectionManagerEventHandler, this), m_SelectionHandlerUnsubscriber);
}

void ezSceneDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  // (Local mirror only mirrors settings)
  m_ObjectMirror.SetFilterFunction([pManager = GetObjectManager()](const ezDocumentObject* pObject, ezStringView sProperty) -> bool
    { return pManager->IsUnderRootProperty("Settings", pObject, sProperty); });
  // (Remote IPC mirror only sends scene)
  m_pMirror->SetFilterFunction([pManager = GetObjectManager()](const ezDocumentObject* pObject, ezStringView sProperty) -> bool
    { return pManager->IsUnderRootProperty("Children", pObject, sProperty); });

  EnsureSettingsObjectExist();

  m_DocumentObjectMetaData->m_DataModifiedEvent.AddEventHandler(ezMakeDelegate(&ezSceneDocument::DocumentObjectMetaDataEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ToolsProjectEventHandler, this));
  ezEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocument::EngineConnectionEventHandler, this));

  m_ObjectMirror.InitSender(GetObjectManager());
  m_ObjectMirror.InitReceiver(&m_Context);
  m_ObjectMirror.SendDocument();
}

ezSceneDocument::~ezSceneDocument()
{
  m_SelectionHandlerUnsubscriber.Unsubscribe();

  m_DocumentObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::DocumentObjectMetaDataEventHandler, this));

  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ToolsProjectEventHandler, this));

  ezEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::EngineConnectionEventHandler, this));

  m_ObjectMirror.Clear();
  m_ObjectMirror.DeInit();
}

void ezSceneDocument::GroupSelection()
{
  const auto& sel = GetSelectionManager()->GetSelection();
  const ezUInt32 numSel = sel.GetCount();
  if (numSel <= 1)
    return;

  const ezDocumentObject* pCommonParent = sel[0]->GetParent();

  // this happens for top-level objects, their parent object is an ezDocumentRootObject
  if (pCommonParent->GetType() != ezGetStaticRTTI<ezGameObject>())
  {
    pCommonParent = nullptr;
  }

  const ezTransform tGroup = GetGlobalTransform(GetSelectionManager()->GetCurrentObject());

  for (const auto& item : sel)
  {
    if (pCommonParent != item->GetParent())
    {
      pCommonParent = nullptr;
    }
  }

  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Group Selection");

  ezUuid groupObj = ezUuid::MakeUuid();

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_NewObjectGuid = groupObj;
  cmdAdd.m_pType = ezGetStaticRTTI<ezGameObject>();
  cmdAdd.m_Index = -1;
  cmdAdd.m_sParentProperty = "Children";

  pHistory->AddCommand(cmdAdd).AssertSuccess();

  // put the new group object under the shared parent
  if (pCommonParent != nullptr)
  {
    ezMoveObjectCommand cmdMove;
    cmdMove.m_NewParent = pCommonParent->GetGuid();
    cmdMove.m_Index = -1;
    cmdMove.m_sParentProperty = "Children";

    cmdMove.m_Object = cmdAdd.m_NewObjectGuid;
    pHistory->AddCommand(cmdMove).AssertSuccess();
  }

  auto pGroupObject = GetObjectManager()->GetObject(cmdAdd.m_NewObjectGuid);
  SetGlobalTransform(pGroupObject, tGroup, TransformationChanges::All);

  ezMoveObjectCommand cmdMove;
  cmdMove.m_NewParent = cmdAdd.m_NewObjectGuid;
  cmdMove.m_Index = -1;
  cmdMove.m_sParentProperty = "Children";

  for (const auto& item : sel)
  {
    cmdMove.m_Object = item->GetGuid();
    pHistory->AddCommand(cmdMove).AssertSuccess();
  }

  pHistory->FinishTransaction();

  const ezDocumentObject* pGroupObj = GetObjectManager()->GetObject(groupObj);

  GetSelectionManager()->SetSelection(pGroupObj);

  ShowDocumentStatus(ezFmt("Grouped {} objects", numSel));
}

void ezSceneDocument::SelectParentObject()
{
  const auto& Sel = GetSelectionManager()->GetSelection();

  if (Sel.IsEmpty())
    return;

  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

  const ezDocumentObject* pObject = GetObjectManager()->GetObject(Sel[0]->GetGuid());

  if (pObject->GetParent() && pObject->GetParent() != GetObjectManager()->GetRootObject())
  {
    GetSelectionManager()->SetSelection(pObject->GetParent());
  }
  else
  {
    ShowDocumentStatus("Object has no parent.");
  }
}

void ezSceneDocument::SetSelectedAsActiveParent()
{
  const auto& sel = GetSelectionManager()->GetSelection();

  if (sel.IsEmpty())
    return;

  SetActiveParent(sel.PeekBack()->GetGuid());
}

void ezSceneDocument::ClearActiveParent()
{
  SetActiveParent(ezUuid::MakeInvalid());
}

void ezSceneDocument::DuplicateSpecial()
{
  if (GetSelectionManager()->IsSelectionEmpty())
    return;

  ezQtDuplicateDlg dlg(nullptr);
  if (dlg.exec() == QDialog::Rejected)
    return;

  ezMap<ezUuid, ezUuid> parents;

  ezAbstractObjectGraph graph;
  CopySelectedObjects(graph, &parents);

  ezStringBuilder temp, tmp1, tmp2;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", ezConversionUtils::ToString(it.Key(), tmp1), ezConversionUtils::ToString(it.Value(), tmp2));
  }

  // Serialize to string
  ezContiguousMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  ezAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

  ezDuplicateObjectsCommand cmd;
  cmd.m_sGraphTextFormat = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes = temp;
  cmd.m_uiNumberOfCopies = dlg.s_uiNumberOfCopies;
  cmd.m_vAccumulativeTranslation = dlg.s_vTranslationStep;
  cmd.m_vAccumulativeRotation = dlg.s_vRotationStep;
  cmd.m_vRandomRotation = dlg.s_vRandomRotation;
  cmd.m_vRandomTranslation = dlg.s_vRandomTranslation;
  cmd.m_bGroupDuplicates = dlg.s_bGroupCopies;
  cmd.m_iRevolveAxis = dlg.s_iRevolveAxis;
  cmd.m_fRevolveRadius = dlg.s_fRevolveRadius;
  cmd.m_RevolveStartAngle = ezAngle::MakeFromDegree(dlg.s_iRevolveStartAngle);
  cmd.m_RevolveAngleStep = ezAngle::MakeFromDegree(dlg.s_iRevolveAngleStep);

  auto history = GetCommandHistory();

  history->StartTransaction("Duplicate Special");

  if (history->AddCommand(cmd).m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}


void ezSceneDocument::DeltaTransform()
{
  if (GetSelectionManager()->IsSelectionEmpty())
    return;

  ezQtDeltaTransformDlg dlg(nullptr, this);
  dlg.exec();
}

void ezSceneDocument::SnapObjectToCamera()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr)
    return;

  if (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
  {
    ShowDocumentStatus("Note: This operation can only be performed in perspective views.");
    return;
  }

  const auto& camera = ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  ezMat3 mRot;

  ezTransform transform;
  transform.m_vScale.Set(1.0f);
  transform.m_vPosition = camera.GetCenterPosition();
  mRot.SetColumn(0, camera.GetCenterDirForwards());
  mRot.SetColumn(1, camera.GetCenterDirRight());
  mRot.SetColumn(2, camera.GetCenterDirUp());
  transform.m_qRotation = ezQuat::MakeFromMat3(mRot);

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Snap Object to Camera");
  {
    for (const ezDocumentObject* pObject : selection)
    {
      SetGlobalTransform(pObject, transform, TransformationChanges::Translation | TransformationChanges::Rotation);
    }
  }
  pHistory->FinishTransaction();
}


void ezSceneDocument::AttachToObject()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();
  if (ctxt.m_pLastHoveredViewWidget == nullptr || ctxt.m_pLastPickingResult == nullptr || !ctxt.m_pLastPickingResult->m_PickedObject.IsValid())
    return;

  if (GetObjectManager()->GetObject(ctxt.m_pLastPickingResult->m_PickedObject) == nullptr)
  {
    ezQtUiServices::GetSingleton()->MessageBoxStatus(ezStatus(EZ_FAILURE), "Target object belongs to a different document.");
    return;
  }

  ezMoveObjectCommand cmd;
  cmd.m_sParentProperty = "Children";
  cmd.m_NewParent = ctxt.m_pLastPickingResult->m_PickedObject;
  cmd.m_Index = -1;

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Attach to Object");
  {
    for (const ezDocumentObject* pObject : selection)
    {
      cmd.m_Object = pObject->GetGuid();

      auto res = pHistory->AddCommand(cmd);
      if (res.Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Attach to object failed");
        pHistory->CancelTransaction();
        return;
      }
    }
  }
  pHistory->FinishTransaction();
}

void ezSceneDocument::DetachFromParent()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  ezMoveObjectCommand cmd;
  cmd.m_sParentProperty = "Children";

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Detach from Parent");
  {
    for (const ezDocumentObject* pObject : selection)
    {
      cmd.m_Object = pObject->GetGuid();

      auto res = pHistory->AddCommand(cmd);
      if (res.Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Detach from parent failed");
        pHistory->CancelTransaction();
        return;
      }
    }
  }
  pHistory->FinishTransaction();

  ShowDocumentStatus(ezFmt("Detached {} objects", selection.GetCount()));

  // reapply the selection to fix tree views etc. after the re-parenting
  ezDeque<const ezDocumentObject*> prevSelection = selection;
  GetSelectionManager()->Clear();
  GetSelectionManager()->SetSelection(prevSelection);
}

void ezSceneDocument::CopyReference()
{
  if (GetSelectionManager()->GetSelection().GetCount() != 1)
    return;

  const ezUuid guid = GetSelectionManager()->GetSelection()[0]->GetGuid();

  ezStringBuilder sGuid;
  ezConversionUtils::ToString(guid, sGuid);

  QApplication::clipboard()->setText(sGuid.GetData());

  ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(ezFmt("Copied Object Reference: {}", sGuid), ezTime::MakeFromSeconds(5));
}

ezStatus ezSceneDocument::CreateEmptyObject(bool bAttachToParent, bool bAtPickedPosition, bool bComponentSelectionMenu)
{
  const ezRTTI* pComponentType = ezRTTI::FindTypeByName("ezShapeIconComponent");

  if (bComponentSelectionMenu)
  {
    // show the context menu to select a component type

    QMenu m;
    ezQtTypeMenu tm;
    tm.FillMenu(&m, ezGetStaticRTTI<ezComponent>(), true, false);

    m.exec(QCursor::pos());

    if (tm.m_pLastSelectedType)
    {
      pComponentType = tm.m_pLastSelectedType;
      tm.m_pLastSelectedType = nullptr;
    }
  }

  auto history = GetCommandHistory();

  history->StartTransaction("Create Node");

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_pType = ezGetStaticRTTI<ezGameObject>();
  cmdAdd.m_sParentProperty = "Children";
  cmdAdd.m_Index = -1;

  ezUuid NewNode;

  const auto& Sel = GetSelectionManager()->GetSelection();

  if (Sel.IsEmpty() || !bAttachToParent)
  {
    cmdAdd.m_NewObjectGuid = ezUuid::MakeUuid();
    NewNode = cmdAdd.m_NewObjectGuid;

    if (!bAttachToParent)
    {
      const ezUuid activeParent = GetRedirectedGameObjectDoc()->GetActiveParent();

      // the object may not exist anymore
      if (auto pParentObj = GetObjectManager()->GetObject(activeParent))
      {
        cmdAdd.m_Parent = activeParent;
      }
    }

    auto res = history->AddCommand(cmdAdd);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }
  else
  {
    cmdAdd.m_NewObjectGuid = ezUuid::MakeUuid();
    NewNode = cmdAdd.m_NewObjectGuid;

    cmdAdd.m_Parent = Sel[0]->GetGuid();
    auto res = history->AddCommand(cmdAdd);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }

  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

  if (!bAttachToParent && bAtPickedPosition && ctxt.m_pLastPickingResult && !ctxt.m_pLastPickingResult->m_vPickedPosition.IsNaN())
  {
    ezVec3 position = ctxt.m_pLastPickingResult->m_vPickedPosition;

    ezSnapProvider::SnapTranslation(position);

    ezSetObjectPropertyCommand cmdSet;
    cmdSet.m_NewValue = position;
    cmdSet.m_Object = NewNode;
    cmdSet.m_sProperty = "LocalPosition";

    if (auto pParentObj = GetObjectManager()->GetObject(cmdAdd.m_Parent))
    {
      const ezTransform tParent = GetGlobalTransform(pParentObj);
      const ezTransform tRel = ezTransform::MakeLocalTransform(tParent, ezTransform(position, ezQuat::MakeIdentity()));

      cmdSet.m_NewValue = tRel.m_vPosition;
    }

    auto res = history->AddCommand(cmdSet);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }

  // Add a dummy shape icon component, which enables picking
  {
    ezAddObjectCommand cmdAdd;
    cmdAdd.m_pType = pComponentType;
    cmdAdd.m_sParentProperty = "Components";
    cmdAdd.m_Index = -1;
    cmdAdd.m_Parent = NewNode;

    auto res = history->AddCommand(cmdAdd);
  }

  history->FinishTransaction();

  GetSelectionManager()->SetSelection(GetObjectManager()->GetObject(NewNode));
  return ezStatus(EZ_SUCCESS);
}

void ezSceneDocument::DuplicateSelection()
{
  ezMap<ezUuid, ezUuid> parents;

  ezAbstractObjectGraph graph;
  CopySelectedObjects(graph, &parents);

  ezStringBuilder temp, tmp1, tmp2;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", ezConversionUtils::ToString(it.Key(), tmp1), ezConversionUtils::ToString(it.Value(), tmp2));
  }

  // Serialize to string
  ezContiguousMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  ezAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

  ezDuplicateObjectsCommand cmd;
  cmd.m_sGraphTextFormat = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes = temp;

  auto history = GetCommandHistory();

  history->StartTransaction("Duplicate Selection");

  if (history->AddCommand(cmd).m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}

void ezSceneDocument::ShowOrHideSelectedObjects(ShowOrHide action)
{
  const bool bHide = action == ShowOrHide::Hide;

  auto sel = GetSelectionManager()->GetSelection();

  for (auto pItem : sel)
  {
    if (!pItem->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
      continue;

    ApplyRecursive(pItem, [this, bHide](const ezDocumentObject* pObj)
      {
      auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObj->GetGuid());
      if (pMeta->m_bHidden != bHide)
      {
        pMeta->m_bHidden = bHide;
        m_DocumentObjectMetaData->EndModifyMetaData(ezDocumentObjectMetaData::HiddenFlag);
      }
      else
        m_DocumentObjectMetaData->EndModifyMetaData(0); });
  }
}

void ezSceneDocument::HideUnselectedObjects()
{
  ShowOrHideAllObjects(ShowOrHide::Hide);

  ShowOrHideSelectedObjects(ShowOrHide::Show);
}

void ezSceneDocument::SetGameMode(GameMode::Enum mode)
{
  if (m_GameMode == mode)
    return;

  // store settings of recently active mode
  m_GameModeData[m_GameMode] = m_CurrentMode;

  m_GameMode = mode;

  switch (m_GameMode)
  {
    case GameMode::Off:
      ShowDocumentStatus("Game Mode: Off");
      break;
    case GameMode::Simulate:
      ShowDocumentStatus("Game Mode: Simulate");
      break;
    case GameMode::Play:
      ShowDocumentStatus("Game Mode: Play");
      break;
  }

  SetRenderSelectionOverlay(m_GameModeData[m_GameMode].m_bRenderSelectionOverlay);
  SetRenderShapeIcons(m_GameModeData[m_GameMode].m_bRenderShapeIcons);
  SetRenderVisualizers(m_GameModeData[m_GameMode].m_bRenderVisualizers);

  if (m_GameMode == GameMode::Off)
  {
    // reset the game world
    SendGameWorldToEngine();
  }

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::GameModeChanged;
  m_GameObjectEvents.Broadcast(e);

  ScheduleSendObjectSelection();
}

ezStatus ezSceneDocument::CreatePrefabDocumentFromSelection(ezStringView sFile, const ezRTTI* pRootType, ezDelegate<void(ezAbstractObjectNode*)> adjustGraphNodeCB /* = {} */, ezDelegate<void(ezDocumentObject*)> adjustNewNodesCB /* = {} */, ezDelegate<void(ezAbstractObjectGraph& graph, ezDynamicArray<ezAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB /* = {} */)
{
  ezHybridArray<ezSelectionEntry, 32> Selection;
  GetSelectionManager()->GetTopLevelSelectionOfType(pRootType, Selection);

  if (Selection.IsEmpty())
    return ezStatus("To create a prefab, the selection must not be empty");

  const ezTransform tReference = QueryLocalTransform(Selection.PeekBack().m_pObject);

  ezVariantArray varChildren;

  auto centerNodes = [tReference, &varChildren](ezAbstractObjectNode* pGraphNode)
  {
    if (auto pPosition = pGraphNode->FindProperty("LocalPosition"))
    {
      ezVec3 pos = pPosition->m_Value.ConvertTo<ezVec3>();
      pos -= tReference.m_vPosition;

      pGraphNode->ChangeProperty("LocalPosition", pos);
    }

    if (auto pRotation = pGraphNode->FindProperty("LocalRotation"))
    {
      ezQuat rot = pRotation->m_Value.ConvertTo<ezQuat>();
      rot = tReference.m_qRotation.GetInverse() * rot;

      pGraphNode->ChangeProperty("LocalRotation", rot);
    }

    varChildren.PushBack(pGraphNode->GetGuid());
  };

  auto adjustResult = [tReference, this](ezDocumentObject* pObject)
  {
    const ezTransform tOld = QueryLocalTransform(pObject);

    ezSetObjectPropertyCommand cmd;
    cmd.m_Object = pObject->GetGuid();

    cmd.m_sProperty = "LocalPosition";
    cmd.m_NewValue = tOld.m_vPosition + tReference.m_vPosition;
    GetCommandHistory()->AddCommand(cmd).AssertSuccess();

    cmd.m_sProperty = "LocalRotation";
    cmd.m_NewValue = tReference.m_qRotation * tOld.m_qRotation;
    GetCommandHistory()->AddCommand(cmd).AssertSuccess();
  };

  auto finalizeGraph = [this, &varChildren](ezAbstractObjectGraph& ref_graph, ezDynamicArray<ezAbstractObjectNode*>& ref_graphRootNodes)
  {
    if (ref_graphRootNodes.GetCount() == 1)
    {
      ref_graphRootNodes[0]->ChangeProperty("Name", "<Prefab-Root>");
    }
    else
    {
      const ezRTTI* pRtti = ezGetStaticRTTI<ezGameObject>();

      ezAbstractObjectNode* pRoot = ref_graph.AddNode(ezUuid::MakeUuid(), pRtti->GetTypeName(), pRtti->GetTypeVersion());
      pRoot->AddProperty("Name", "<Prefab-Root>");
      pRoot->AddProperty("Children", varChildren);

      ref_graphRootNodes.Clear();
      ref_graphRootNodes.PushBack(pRoot);
    }
  };

  if (!adjustGraphNodeCB.IsValid())
    adjustGraphNodeCB = centerNodes;
  if (!adjustNewNodesCB.IsValid())
    adjustNewNodesCB = adjustResult;
  if (!finalizeGraphCB.IsValid())
    finalizeGraphCB = finalizeGraph;

  return SUPER::CreatePrefabDocumentFromSelection(sFile, pRootType, adjustGraphNodeCB, adjustNewNodesCB, finalizeGraphCB);
}

bool ezSceneDocument::CanEngineProcessBeRestarted() const
{
  return m_GameMode == GameMode::Off;
}

void ezSceneDocument::StartSimulateWorld()
{
  if (m_GameMode != GameMode::Off)
  {
    StopGameMode();
    return;
  }

  {
    ezGameObjectDocumentEvent e;
    e.m_Type = ezGameObjectDocumentEvent::Type::GameMode_StartingSimulate;
    e.m_pDocument = this;
    s_GameObjectDocumentEvents.Broadcast(e);
  }

  SetGameMode(GameMode::Simulate);
}


void ezSceneDocument::TriggerGameModePlay(bool bUsePickedPositionAsStart)
{
  if (m_GameMode != GameMode::Off)
  {
    StopGameMode();
    return;
  }

  {
    ezGameObjectDocumentEvent e;
    e.m_Type = ezGameObjectDocumentEvent::Type::GameMode_StartingPlay;
    e.m_pDocument = this;
    s_GameObjectDocumentEvents.Broadcast(e);
  }

  UpdateObjectDebugTargets();

  // attempt to start PTG
  // do not change state here
  {
    ezGameModeMsgToEngine msg;
    msg.m_bEnablePTG = true;
    msg.m_bUseStartPosition = false;

    if (bUsePickedPositionAsStart)
    {
      const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

      if (ctxt.m_pLastHoveredViewWidget != nullptr && ctxt.m_pLastHoveredViewWidget->GetDocumentWindow()->GetDocument() == this)
      {
        msg.m_bUseStartPosition = true;
        msg.m_vStartPosition = ctxt.m_pLastPickingResult->m_vPickedPosition;

        ezVec3 vPickDir = ctxt.m_pLastPickingResult->m_vPickedPosition - ctxt.m_pLastPickingResult->m_vPickingRayStart;
        vPickDir.z = 0;
        vPickDir.NormalizeIfNotZero(ezVec3(1, 0, 0)).IgnoreResult();

        msg.m_vStartDirection = vPickDir;
      }
    }

    GetEditorEngineConnection()->SendMessage(&msg);
  }
}


bool ezSceneDocument::StopGameMode()
{
  if (m_GameMode == GameMode::Off)
    return false;

  if (m_GameMode == GameMode::Simulate)
  {
    // we can set that state immediately
    SetGameMode(GameMode::Off);
  }

  if (m_GameMode == GameMode::Play)
  {
    // attempt to stop PTG
    // do not change any state, that will be done by the response msg
    {
      ezGameModeMsgToEngine msg;
      msg.m_bEnablePTG = false;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
  }

  {
    ezGameObjectDocumentEvent e;
    e.m_Type = ezGameObjectDocumentEvent::Type::GameMode_Stopped;
    e.m_pDocument = this;
    s_GameObjectDocumentEvents.Broadcast(e);
  }

  return true;
}

void ezSceneDocument::ShowOrHideAllObjects(ShowOrHide action)
{
  const bool bHide = action == ShowOrHide::Hide;

  ApplyRecursive(GetObjectManager()->GetRootObject(), [this, bHide](const ezDocumentObject* pObj)
    {
    // if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    // return;

    ezUInt32 uiFlags = 0;

    auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObj->GetGuid());

    if (pMeta->m_bHidden != bHide)
    {
      pMeta->m_bHidden = bHide;
      uiFlags = ezDocumentObjectMetaData::HiddenFlag;
    }

    m_DocumentObjectMetaData->EndModifyMetaData(uiFlags); });
}
void ezSceneDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/ezEditor.ezAbstractGraph");
}

bool ezSceneDocument::CopySelectedObjects(ezAbstractObjectGraph& ref_graph, ezStringBuilder& out_sMimeType) const
{
  out_sMimeType = "application/ezEditor.ezAbstractGraph";
  return CopySelectedObjects(ref_graph, nullptr);
}

bool ezSceneDocument::CopySelectedObjects(ezAbstractObjectGraph& ref_graph, ezMap<ezUuid, ezUuid>* out_pParents) const
{
  if (GetSelectionManager()->GetSelection().GetCount() == 0)
    return false;

  // Serialize selection to graph
  ezHybridArray<ezSelectionEntry, 64> selection;
  GetSelectionManager()->GetTopLevelSelection(selection);

  ezDocumentObjectConverterWriter writer(&ref_graph, GetObjectManager());

  // objects are required to be named root but this is not enforced or obvious by the interface.
  for (ezUInt32 i = 0; i < selection.GetCount(); i++)
  {
    const auto& item = selection[i];
    ezAbstractObjectNode* pNode = writer.AddObjectToGraph(item.m_pObject, "root");
    pNode->AddProperty("__GlobalTransform", GetGlobalTransform(item.m_pObject));
    pNode->AddProperty("__Order", i);
    pNode->AddProperty("__SelectionOrder", item.m_uiSelectionOrder);
  }

  if (out_pParents != nullptr)
  {
    out_pParents->Clear();

    for (const auto& item : selection)
    {
      (*out_pParents)[item.m_pObject->GetGuid()] = item.m_pObject->GetParent()->GetGuid();
    }
  }

  AttachMetaDataBeforeSaving(ref_graph);

  return true;
}

bool ezSceneDocument::PasteAt(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, const ezVec3& vPasteAt)
{
  ezTransform refTransform = ezTransform::MakeIdentity();
  ezUInt32 uiHighestSelectionOrder = 0;

  ezHybridArray<ezTransform, 16> globalTransforms;
  globalTransforms.SetCount(info.GetCount(), ezTransform::MakeIdentity());

  for (ezUInt32 i = 0; i < info.GetCount(); ++i)
  {
    const PasteInfo& pi = info[i];

    if (pi.m_pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezGameObject>())
      return false;

    if (auto* pNode = objectGraph.GetNode(pi.m_pObject->GetGuid()))
    {
      if (auto* pProperty = pNode->FindProperty("__GlobalTransform"))
      {
        globalTransforms[i] = pProperty->m_Value.Get<ezTransform>();

        if (auto* pProperty = pNode->FindProperty("__SelectionOrder"))
        {
          // find the last selected element, and use it as the reference point for the paste position

          const ezUInt32 uiSelOrder = pProperty->m_Value.ConvertTo<ezUInt32>();
          if (uiSelOrder >= uiHighestSelectionOrder)
          {
            uiHighestSelectionOrder = uiSelOrder;
            refTransform = globalTransforms[i];
          }
        }
      }
    }
  }

  for (ezUInt32 i = 0; i < info.GetCount(); ++i)
  {
    const PasteInfo& pi = info[i];

    if (pi.m_pParent == nullptr || pi.m_pParent == GetObjectManager()->GetRootObject())
    {
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", pi.m_Index);
    }

    ezTransform tNew = globalTransforms[i];
    tNew.m_vPosition -= refTransform.m_vPosition;
    tNew.m_vPosition += vPasteAt;

    SetGlobalTransform(pi.m_pObject, tNew, TransformationChanges::All);
  }

  return true;
}

bool ezSceneDocument::PasteAtOrignalPosition(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph)
{
  for (const PasteInfo& pi : info)
  {
    if (pi.m_pParent == nullptr || pi.m_pParent == GetObjectManager()->GetRootObject())
    {
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", pi.m_Index);
    }

    if (auto* pNode = objectGraph.GetNode(pi.m_pObject->GetGuid()))
    {
      if (auto* pProperty = pNode->FindProperty("__GlobalTransform"))
      {
        if (pProperty->m_Value.IsA<ezTransform>())
        {
          SetGlobalTransform(pi.m_pObject, pProperty->m_Value.Get<ezTransform>(), TransformationChanges::All);
        }
      }
    }
  }

  return true;
}

bool ezSceneDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType)
{
  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

  if (bAllowPickedPosition && ctxt.m_pLastPickingResult && ctxt.m_pLastPickingResult->m_PickedObject.IsValid())
  {
    ezVec3 pos = ctxt.m_pLastPickingResult->m_vPickedPosition;
    ezSnapProvider::SnapTranslation(pos);

    if (!PasteAt(info, objectGraph, pos))
      return false;
  }
  else
  {
    if (!PasteAtOrignalPosition(info, objectGraph))
      return false;
  }

  m_DocumentObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);
  m_GameObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);

  // set the pasted objects as the new selection
  {
    auto pSelMan = GetSelectionManager();

    ezDeque<const ezDocumentObject*> NewSelection;
    NewSelection.SetCount(info.GetCount());

    for (ezUInt32 i = 0; i < info.GetCount(); ++i)
    {
      const PasteInfo& pi = info[i];

      ezUInt32 order = i;
      if (auto* pNode = objectGraph.GetNode(pi.m_pObject->GetGuid()))
      {
        if (auto* pProperty = pNode->FindProperty("__SelectionOrder"))
        {
          order = pProperty->m_Value.ConvertTo<ezUInt32>();
        }
      }

      NewSelection[order] = pi.m_pObject;
    }

    pSelMan->SetSelection(NewSelection);
  }

  return true;
}

bool ezSceneDocument::DuplicateSelectedObjects(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bSetSelected)
{
  if (!PasteAtOrignalPosition(info, objectGraph))
    return false;

  m_DocumentObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);
  m_GameObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);

  // set the pasted objects as the new selection
  if (bSetSelected)
  {
    auto pSelMan = GetSelectionManager();

    ezDeque<const ezDocumentObject*> NewSelection;

    for (const PasteInfo& pi : info)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }

  return true;
}

void ezSceneDocument::EnsureSettingsObjectExist()
{
  // Settings object was changed to have a base class and each document type has a different implementation.
  const ezRTTI* pSettingsType = nullptr;
  switch (m_DocumentType)
  {
    case ezSceneDocument::DocumentType::Scene:
      pSettingsType = ezGetStaticRTTI<ezSceneDocumentSettings>();
      break;
    case ezSceneDocument::DocumentType::Prefab:
      pSettingsType = ezGetStaticRTTI<ezPrefabDocumentSettings>();
      break;
    case ezSceneDocument::DocumentType::Layer:
      pSettingsType = ezGetStaticRTTI<ezLayerDocumentSettings>();
      break;
  }

  auto pRoot = GetObjectManager()->GetRootObject();
  // Use the ezObjectDirectAccessor instead of calling GetObjectAccessor because we do not want
  // undo ops for this operation.
  ezObjectDirectAccessor accessor(GetObjectManager());
  ezVariant value;
  EZ_VERIFY(accessor.ezObjectAccessorBase::GetValueByName(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  ezUuid id = value.Get<ezUuid>();
  if (!id.IsValid())
  {
    EZ_VERIFY(accessor.ezObjectAccessorBase::AddObjectByName(pRoot, "Settings", ezVariant(), pSettingsType, id).Succeeded(), "Adding scene settings object to root failed.");
  }
  else
  {
    ezDocumentObject* pSettings = GetObjectManager()->GetObject(id);
    EZ_VERIFY(pSettings, "Document corrupt, root references a non-existing object");
    if (pSettings->GetType() != pSettingsType)
    {
      accessor.RemoveObject(pSettings).AssertSuccess();
      GetObjectManager()->DestroyObject(pSettings);
      EZ_VERIFY(accessor.ezObjectAccessorBase::AddObjectByName(pRoot, "Settings", ezVariant(), pSettingsType, id).Succeeded(), "Adding scene settings object to root failed.");
    }
  }
}

const ezDocumentObject* ezSceneDocument::GetSettingsObject() const
{
  auto pRoot = GetObjectManager()->GetRootObject();
  ezVariant value;
  EZ_VERIFY(GetObjectAccessor()->GetValueByName(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  ezUuid id = value.Get<ezUuid>();
  return GetObjectManager()->GetObject(id);
}

const ezSceneDocumentSettingsBase* ezSceneDocument::GetSettingsBase() const
{
  return static_cast<const ezSceneDocumentSettingsBase*>(m_ObjectMirror.GetNativeObjectPointer(GetSettingsObject()));
}

ezStatus ezSceneDocument::CreateExposedProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index, ezExposedSceneProperty& out_key) const
{
  const ezDocumentObject* pNodeComponent = ezObjectPropertyPath::FindParentNodeComponent(pObject);
  if (!pObject)
    return ezStatus("No parent node or component found.");

  ezObjectPropertyPathContext context = {pNodeComponent, GetObjectAccessor(), "Children"};
  ezPropertyReference propertyRef = {pObject->GetGuid(), pProperty, index};
  ezStringBuilder sPropertyPath;
  ezStatus res = ezObjectPropertyPath::CreatePropertyPath(context, propertyRef, sPropertyPath);
  if (res.Failed())
    return res;

  out_key.m_Object = pNodeComponent->GetGuid();
  out_key.m_sPropertyPath = sPropertyPath;
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSceneDocument::AddExposedParameter(const char* szName, const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index)
{
  if (m_DocumentType != DocumentType::Prefab)
    return ezStatus("Exposed parameters are only supported in prefab documents.");

  if (FindExposedParameter(pObject, pProperty, index) != -1)
    return ezStatus("Exposed parameter already exists.");

  ezExposedSceneProperty key;
  ezStatus res = CreateExposedProperty(pObject, pProperty, index, key);
  if (res.Failed())
    return res;

  ezUuid id;
  res = GetObjectAccessor()->AddObjectByName(GetSettingsObject(), "ExposedProperties", -1, ezGetStaticRTTI<ezExposedSceneProperty>(), id);
  if (res.Failed())
    return res;
  const ezDocumentObject* pParam = GetObjectManager()->GetObject(id);
  GetObjectAccessor()->SetValueByName(pParam, "Name", szName).LogFailure();
  GetObjectAccessor()->SetValueByName(pParam, "Object", key.m_Object).LogFailure();
  GetObjectAccessor()->SetValueByName(pParam, "PropertyPath", ezVariant(key.m_sPropertyPath)).LogFailure();
  return ezStatus(EZ_SUCCESS);
}

ezInt32 ezSceneDocument::FindExposedParameter(const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index)
{
  EZ_ASSERT_DEV(m_DocumentType == DocumentType::Prefab, "Exposed properties are only supported in prefab documents.");

  ezExposedSceneProperty key;
  ezStatus res = CreateExposedProperty(pObject, pProperty, index, key);
  if (res.Failed())
    return -1;

  const ezPrefabDocumentSettings* settings = GetSettings<ezPrefabDocumentSettings>();
  for (ezUInt32 i = 0; i < settings->m_ExposedProperties.GetCount(); i++)
  {
    const auto& param = settings->m_ExposedProperties[i];
    if (param.m_Object == key.m_Object && param.m_sPropertyPath == key.m_sPropertyPath)
      return (ezInt32)i;
  }
  return -1;
}

ezStatus ezSceneDocument::RemoveExposedParameter(ezInt32 iIndex)
{
  ezVariant value;
  auto res = GetObjectAccessor()->GetValueByName(GetSettingsObject(), "ExposedProperties", value, iIndex);
  if (res.Failed())
    return res;

  ezUuid id = value.Get<ezUuid>();
  return GetObjectAccessor()->RemoveObject(GetObjectManager()->GetObject(id));
}


void ezSceneDocument::StoreFavoriteCamera(ezUInt8 uiSlot)
{
  EZ_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  ezQuadViewPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezQuadViewPreferencesUser>(this);
  auto& cam = pPreferences->m_FavoriteCamera[uiSlot];

  auto* pView = ezQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView)
  {
    const auto& camera = pView->m_pViewConfig->m_Camera;

    cam.m_PerspectiveMode = pView->m_pViewConfig->m_Perspective;
    cam.m_vCamPos = camera.GetCenterPosition();
    cam.m_vCamDir = camera.GetCenterDirForwards();
    cam.m_vCamUp = camera.GetCenterDirUp();

    // make sure the data gets saved
    pPreferences->TriggerPreferencesChangedEvent();
  }
}

void ezSceneDocument::RestoreFavoriteCamera(ezUInt8 uiSlot)
{
  EZ_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  ezQuadViewPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezQuadViewPreferencesUser>(this);
  auto& cam = pPreferences->m_FavoriteCamera[uiSlot];

  auto* pView = ezQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return;

  ezVec3 vCamPos = cam.m_vCamPos;
  ezVec3 vCamDir = cam.m_vCamDir;
  ezVec3 vCamUp = cam.m_vCamUp;

  // if the projection mode of the view is orthographic, ignore the direction of the stored favorite camera
  // if we apply a favorite that was saved in an orthographic view, and we apply it to a perspective view,
  // we want to ignore one of the axis, as the respective orthographic position can be arbitrary
  switch (pView->m_pViewConfig->m_Perspective)
  {
    case ezSceneViewPerspective::Orthogonal_Front:
    case ezSceneViewPerspective::Orthogonal_Right:
    case ezSceneViewPerspective::Orthogonal_Top:
      vCamDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards();
      vCamUp = pView->m_pViewConfig->m_Camera.GetCenterDirUp();
      break;

    case ezSceneViewPerspective::Perspective:
    {
      const ezVec3 vOldPos = pView->m_pViewConfig->m_Camera.GetCenterPosition();

      switch (cam.m_PerspectiveMode)
      {
        case ezSceneViewPerspective::Orthogonal_Front:
          vCamPos.x = vOldPos.x;
          break;
        case ezSceneViewPerspective::Orthogonal_Right:
          vCamPos.y = vOldPos.y;
          break;
        case ezSceneViewPerspective::Orthogonal_Top:
          vCamPos.z = vOldPos.z;
          break;
        case ezSceneViewPerspective::Perspective:
          break;
      }

      break;
    }
  }

  pView->InterpolateCameraTo(vCamPos, vCamDir, pView->m_pViewConfig->m_Camera.GetFovOrDim(), &vCamUp);
}

ezResult ezSceneDocument::JumpToLevelCamera(ezUInt8 uiSlot, bool bImmediate)
{
  EZ_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  auto* pView = ezQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return EZ_FAILURE;

  auto* pObjMan = GetObjectManager();

  ezHybridArray<ezDocumentObject*, 8> stack;
  stack.PushBack(pObjMan->GetRootObject());

  const ezRTTI* pCamType = ezGetStaticRTTI<ezCameraComponent>();
  const ezDocumentObject* pCamObj = nullptr;

  while (!stack.IsEmpty())
  {
    const ezDocumentObject* pObj = stack.PeekBack();
    stack.PopBack();

    stack.PushBackRange(pObj->GetChildren());

    if (pObj->GetType() == pCamType)
    {
      ezInt32 iShortcut = pObj->GetTypeAccessor().GetValue("EditorShortcut").ConvertTo<ezInt32>();

      if (iShortcut == uiSlot)
      {
        pCamObj = pObj->GetParent();
        break;
      }
    }
  }

  if (pCamObj == nullptr)
    return EZ_FAILURE;

  const ezTransform tCam = GetGlobalTransform(pCamObj);

  ezVec3 vCamDir = tCam.m_qRotation * ezVec3(1, 0, 0);
  ezVec3 vCamUp = tCam.m_qRotation * ezVec3(0, 0, 1);

  // if the projection mode of the view is orthographic, ignore the direction of the level camera
  switch (pView->m_pViewConfig->m_Perspective)
  {
    case ezSceneViewPerspective::Orthogonal_Front:
    case ezSceneViewPerspective::Orthogonal_Right:
    case ezSceneViewPerspective::Orthogonal_Top:
      vCamDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards();
      vCamUp = pView->m_pViewConfig->m_Camera.GetCenterDirUp();
      break;

    case ezSceneViewPerspective::Perspective:
      break;
  }

  pView->InterpolateCameraTo(tCam.m_vPosition, vCamDir, pView->m_pViewConfig->m_Camera.GetFovOrDim(), &vCamUp, bImmediate);

  return EZ_SUCCESS;
}

ezResult ezSceneDocument::CreateLevelCamera(ezUInt8 uiSlot)
{
  EZ_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  auto* pView = ezQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return EZ_FAILURE;

  if (pView->m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
    return EZ_FAILURE;

  const auto* pRootObj = GetObjectManager()->GetRootObject();

  const ezVec3 vPos = pView->m_pViewConfig->m_Camera.GetCenterPosition();
  const ezVec3 vDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards().GetNormalized();
  const ezVec3 vUp = pView->m_pViewConfig->m_Camera.GetCenterDirUp().GetNormalized();

  auto* pAccessor = GetObjectAccessor();
  pAccessor->StartTransaction("Create Level Camera");

  ezUuid camObjGuid;
  if (pAccessor->AddObjectByName(pRootObj, "Children", -1, ezGetStaticRTTI<ezGameObject>(), camObjGuid).Failed())
  {
    pAccessor->CancelTransaction();
    return EZ_FAILURE;
  }

  ezMat3 mRot;
  mRot.SetColumn(0, vDir);
  mRot.SetColumn(1, vUp.CrossRH(vDir).GetNormalized());
  mRot.SetColumn(2, vUp);
  ezQuat qRot;
  qRot = ezQuat::MakeFromMat3(mRot);
  qRot.Normalize();

  SetGlobalTransform(pAccessor->GetObject(camObjGuid), ezTransform(vPos, qRot), TransformationChanges::Translation | TransformationChanges::Rotation);

  ezUuid camCompGuid;
  if (pAccessor->AddObjectByName(pAccessor->GetObject(camObjGuid), "Components", -1, ezGetStaticRTTI<ezCameraComponent>(), camCompGuid).Failed())
  {
    pAccessor->CancelTransaction();
    return EZ_FAILURE;
  }

  if (pAccessor->SetValueByName(pAccessor->GetObject(camCompGuid), "EditorShortcut", uiSlot).Failed())
  {
    pAccessor->CancelTransaction();
    return EZ_FAILURE;
  }

  pAccessor->FinishTransaction();
  return EZ_SUCCESS;
}

void ezSceneDocument::DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & ezDocumentObjectMetaData::HiddenFlag) != 0)
  {
    ezObjectTagMsgToEngine msg;
    msg.m_bSetTag = e.m_pValue->m_bHidden;
    msg.m_sTag = "EditorHidden";
    msg.m_bApplyOnAllChildren = true;

    SendObjectMsg(GetObjectManager()->GetObject(e.m_ObjectKey), &msg);
  }
}

void ezSceneDocument::EngineConnectionEventHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case ezEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    case ezEditorEngineProcessConnection::Event::Type::ProcessShutdown:
    case ezEditorEngineProcessConnection::Event::Type::ProcessStarted:
      SetGameMode(GameMode::Off);
      break;

    default:
      break;
  }
}


void ezSceneDocument::ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezToolsProjectEvent::Type::ProjectConfigChanged:
    {
      // we are lazy and just re-select everything here
      // that ensures that ui elements will rebuild their content

      ezDeque<const ezDocumentObject*> selection = GetSelectionManager()->GetSelection();
      GetSelectionManager()->SetSelection(selection);
    }
    break;

    default:
      break;
  }
}

void ezSceneDocument::HandleGameModeMsg(const ezGameModeMsgToEditor* pMsg)
{
  if (m_GameMode == GameMode::Simulate)
  {
    if (pMsg->m_bRunningPTG)
    {
      m_GameMode = GameMode::Off;
      ezLog::Warning("Incorrect state change from 'simulate' to 'play-the-game'");
    }
    else
    {
      // probably the message just arrived late ?
      return;
    }
  }

  if (m_GameMode == GameMode::Off || m_GameMode == GameMode::Play)
  {
    SetGameMode(pMsg->m_bRunningPTG ? GameMode::Play : GameMode::Off);
    return;
  }

  EZ_REPORT_FAILURE("Unreachable Code reached.");
}

void ezSceneDocument::HandleObjectStateFromEngineMsg(const ezPushObjectStateMsgToEditor* pMsg)
{
  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Pull Object State");

  for (const auto& state : pMsg->m_ObjectStates)
  {
    auto pObject = GetObjectManager()->GetObject(state.m_ObjectGuid);

    if (pObject)
    {
      SetGlobalTransform(pObject, ezTransform(state.m_vPosition, state.m_qRotation), TransformationChanges::Translation | TransformationChanges::Rotation);
    }
  }

  pHistory->FinishTransaction();
}

void ezSceneDocument::SendObjectMsg(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg)
{
  // if ezObjectTagMsgToEngine were derived from a general 'object msg' one could send other message types as well

  if (pObj == nullptr || !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  pMsg->m_ObjectGuid = pObj->GetGuid();
  GetEditorEngineConnection()->SendMessage(pMsg);
}

void ezSceneDocument::SendObjectMsgRecursive(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg)
{
  // if ezObjectTagMsgToEngine were derived from a general 'object msg' one could send other message types as well

  if (pObj == nullptr || !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  pMsg->m_ObjectGuid = pObj->GetGuid();
  GetEditorEngineConnection()->SendMessage(pMsg);

  for (auto pChild : pObj->GetChildren())
  {
    SendObjectMsgRecursive(pChild, pMsg);
  }
}

void ezSceneDocument::GatherObjectsOfType(ezDocumentObject* pRoot, ezGatherObjectsOfTypeMsgInterDoc* pMsg) const
{
  if (pRoot->GetType() == pMsg->m_pType)
  {
    ezStringBuilder sFullPath;
    GenerateFullDisplayName(pRoot, sFullPath);

    auto& res = pMsg->m_Results.ExpandAndGetRef();
    res.m_ObjectGuid = pRoot->GetGuid();
    res.m_pDocument = this;
    res.m_sDisplayName = sFullPath;
  }

  for (auto pChild : pRoot->GetChildren())
  {
    GatherObjectsOfType(pChild, pMsg);
  }
}

void ezSceneDocument::SelectionManagerEventHandler(const ezSelectionManagerEvent& e)
{
  if (!m_bStoreSelectionChange)
    return;

  if (m_iAllowSelectionChanges != -1)
  {
    if (m_iAllowSelectionChanges == 0)
      m_SelectionStack.PopBack();

    --m_iAllowSelectionChanges;
  }

  switch (e.m_Type)
  {
    case ezSelectionManagerEvent::Type::ObjectAdded:
    case ezSelectionManagerEvent::Type::ObjectRemoved:
    case ezSelectionManagerEvent::Type::SelectionSet:
    case ezSelectionManagerEvent::Type::SelectionCleared: // empty selections are important to keep, for layer changes to be undoable
    {
      const auto& curSel = GetSelectionManager()->GetSelection();

      auto& sel = m_SelectionStack.ExpandAndGetRef();

      sel.m_documentGuid = GetRedirectedGameObjectDoc()->GetGuid();

      sel.m_Objects.SetCountUninitialized(curSel.GetCount());

      for (ezUInt32 i = 0; i < curSel.GetCount(); ++i)
      {
        sel.m_Objects[i] = curSel[i]->GetGuid();
      }

      // discard duplicate selection changes (but keep empty selections)
      if (m_SelectionStack.GetCount() > 1)
      {
        const auto& prev = m_SelectionStack[m_SelectionStack.GetCount() - 2];

        if (prev.m_Objects == sel.m_Objects && prev.m_documentGuid == sel.m_documentGuid)
        {
          m_SelectionStack.PopBack();
        }
      }

      if (m_SelectionStack.GetCount() > 16)
      {
        m_SelectionStack.PopFront();
      }

      break;
    }

    default:
      break;
  }
}

bool ezSceneDocument::CanUndoSelection() const
{
  return m_SelectionStack.GetCount() > 1;
}

void ezSceneDocument::UndoSelection()
{
  if (m_SelectionStack.IsEmpty())
    return;

  if (m_SelectionStack.GetCount() > 1)
    m_SelectionStack.PopBack();

  auto& back = m_SelectionStack.PeekBack();

  m_bStoreSelectionChange = false;
  EZ_SCOPE_EXIT(m_bStoreSelectionChange = true);

  auto* pDoc = ezDocumentManager::GetDocumentByGuid(back.m_documentGuid);
  if (pDoc == nullptr)
    return;

  auto pObjMan = pDoc->GetObjectManager();

  ezDeque<const ezDocumentObject*> newSel;
  for (const ezUuid& guid : back.m_Objects)
  {
    if (auto pDoc = pObjMan->GetObject(guid))
    {
      newSel.PushBack(pDoc);
    }
  }

  GetSelectionManager()->SetSelection(newSel);
}

void ezSceneDocument::OnInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender)
{
  // #TODO needs to be overwritten by Scene2
  if (pMessage->GetDynamicRTTI()->IsDerivedFrom<ezGatherObjectsOfTypeMsgInterDoc>())
  {
    GatherObjectsOfType(GetObjectManager()->GetRootObject(), static_cast<ezGatherObjectsOfTypeMsgInterDoc*>(pMessage));
  }
}

ezStatus ezSceneDocument::RequestExportScene(const char* szTargetFile, const ezAssetFileHeader& header)
{
  if (GetGameMode() != GameMode::Off)
    return ezStatus("Cannot export while the scene is simulating");

  EZ_SUCCEED_OR_RETURN(SaveDocument());

  const ezStatus status = ezAssetDocument::RemoteExport(header, szTargetFile);

  // make sure the world is reset
  SendGameWorldToEngine();

  return status;
}

void ezSceneDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  // scenes do not have exposed parameters
  if (!IsPrefab())
    return;

  ezExposedParameters* pExposedParams = EZ_DEFAULT_NEW(ezExposedParameters);

  if (m_DocumentType == DocumentType::Prefab)
  {
    ezSet<ezString> alreadyExposed;

    auto pSettings = GetSettings<ezPrefabDocumentSettings>();
    for (auto prop : pSettings->m_ExposedProperties)
    {
      auto pRootObject = GetObjectManager()->GetObject(prop.m_Object);
      if (!pRootObject)
      {
        ezLog::Warning("The exposed scene property '{0}' does not point to a valid object and is skipped.", prop.m_sName);
        continue;
      }

      ezObjectPropertyPathContext context = {pRootObject, GetObjectAccessor(), "Children"};

      ezPropertyReference key;
      auto res = ezObjectPropertyPath::ResolvePropertyPath(context, prop.m_sPropertyPath, key);
      if (res.Failed())
      {
        ezLog::Warning("The exposed scene property '{0}' can no longer be resolved and is skipped.", prop.m_sName);
        continue;
      }
      ezVariant value;

      const ezExposedParameter* pSourceParameter = nullptr;
      auto pLeafObject = GetObjectManager()->GetObject(key.m_Object);
      if (const ezExposedParametersAttribute* pAttrib = key.m_pProperty->GetAttributeByType<ezExposedParametersAttribute>())
      {
        // If the target of the exposed parameter is yet another exposed parameter, we need to do the following:
        // A: Get the default value from via an ezExposedParameterCommandAccessor. This ensures that in case the target does not actually exist (because it was not overwritten in this template instance) we get the default parameter of the exposed param instead.
        // B: Replace the property type of the exposed parameter (which will always be an ezVariant inside the ezVariantDictionary) with the property type the exposed parameter actually points to.
        const ezAbstractProperty* pParameterSourceProp = pLeafObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
        EZ_ASSERT_DEBUG(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(), pLeafObject->GetType()->GetTypeName());

        ezExposedParameterCommandAccessor proxy(context.m_pAccessor, key.m_pProperty, pParameterSourceProp);
        res = proxy.GetValue(pLeafObject, key.m_pProperty, value, key.m_Index);
        if (key.m_Index.IsA<ezString>())
        {
          pSourceParameter = proxy.GetExposedParam(pLeafObject, key.m_Index.Get<ezString>());
        }
      }
      else
      {
        res = context.m_pAccessor->GetValue(pLeafObject, key.m_pProperty, value, key.m_Index);
      }
      EZ_ASSERT_DEBUG(res.Succeeded(), "ResolvePropertyPath succeeded so GetValue should too");

      // do not show the same parameter twice, even if they have different types, as the UI doesn't handle that case properly
      // TODO: we should prevent users from using the same name for differently typed parameters
      // don't do this earlier, we do want to validate each exposed property with the code above
      if (alreadyExposed.Contains(prop.m_sName))
        continue;

      alreadyExposed.Insert(prop.m_sName);

      ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = prop.m_sName;
      param->m_DefaultValue = value;
      if (pSourceParameter)
      {
        // Copy properties of exposed parameter that we are exposing one level deeper.
        param->m_sType = pSourceParameter->m_sType;
        for (auto attrib : pSourceParameter->m_Attributes)
        {
          param->m_Attributes.PushBack(ezReflectionSerializer::Clone(attrib));
        }
      }
      else
      {
        param->m_sType = key.m_pProperty->GetSpecificType()->GetTypeName();
        for (auto attrib : key.m_pProperty->GetAttributes())
        {
          param->m_Attributes.PushBack(ezReflectionSerializer::Clone(attrib));
        }
      }
    }
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

ezTransformStatus ezSceneDocument::ExportScene(bool bCreateThumbnail)
{
  if (GetUnknownObjectTypeInstances() > 0)
  {
    return ezTransformStatus("Can't export scene/prefab when it contains unknown object types.");
  }

  // #TODO export layers
  auto saveres = SaveDocument();

  if (saveres.m_Result.Failed())
    return saveres;

  ezTransformStatus res;

  if (bCreateThumbnail)
  {
    // this is needed to generate a scene thumbnail, however that has a larger overhead (1 sec or so)
    res = ezAssetCurator::GetSingleton()->TransformAsset(GetGuid(), ezTransformFlags::ForceTransform | ezTransformFlags::TriggeredManually);
  }
  else
    res = TransformAsset(ezTransformFlags::ForceTransform | ezTransformFlags::TriggeredManually);

  if (res.Failed())
    ezLog::Error(res.m_sMessage);
  else
    ezLog::Success(res.m_sMessage);

  ShowDocumentStatus(res.m_sMessage.GetData());

  return res;
}

void ezSceneDocument::ExportSceneGeometry(const char* szFile, bool bOnlySelection, int iExtractionMode, const ezMat3& mTransform)
{
  ezExportSceneGeometryMsgToEngine msg;
  msg.m_sOutputFile = szFile;
  msg.m_bSelectionOnly = bOnlySelection;
  msg.m_iExtractionMode = iExtractionMode;
  msg.m_Transform = mTransform;

  SendMessageToEngine(&msg);

  ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(ezFmt("Geometry exported to '{0}'", szFile), ezTime::MakeFromSeconds(5.0f));
}

void ezSceneDocument::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  ezGameObjectDocument::HandleEngineMessage(pMsg);

  if (const ezGameModeMsgToEditor* msg = ezDynamicCast<const ezGameModeMsgToEditor*>(pMsg))
  {
    HandleGameModeMsg(msg);
    return;
  }

  if (const ezDocumentOpenResponseMsgToEditor* msg = ezDynamicCast<const ezDocumentOpenResponseMsgToEditor*>(pMsg))
  {
    SyncObjectHiddenState();
  }

  if (const ezPushObjectStateMsgToEditor* msg = ezDynamicCast<const ezPushObjectStateMsgToEditor*>(pMsg))
  {
    HandleObjectStateFromEngineMsg(msg);
  }
}

ezTransformStatus ezSceneDocument::InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  if (m_DocumentType == DocumentType::Prefab)
  {
    const ezPrefabDocumentSettings* pSettings = GetSettings<ezPrefabDocumentSettings>();

    if (GetEditorEngineConnection() != nullptr)
    {
      ezExposedDocumentObjectPropertiesMsgToEngine msg;
      msg.m_Properties = pSettings->m_ExposedProperties;

      SendMessageToEngine(&msg);
    }
  }
  return RequestExportScene(szTargetFile, AssetHeader);
}


ezTransformStatus ezSceneDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  EZ_ASSERT_NOT_IMPLEMENTED;

  /* this function is never called */
  return ezStatus(EZ_FAILURE);
}


ezTransformStatus ezSceneDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo, {});

  // if we were to do this BEFORE making the screenshot, the scene would be killed, but not immediately restored
  // and the screenshot would end up empty
  // instead the engine side ensures simulation is stopped and makes a screenshot of whatever state is visible
  // but then the editor and engine state are out of sync, so AFTER the screenshot is done,
  // we ensure to also stop simulation on the editor side
  StopGameMode();

  return status;
}

void ezSceneDocument::SyncObjectHiddenState()
{
  // #TODO Scene2 handling
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}

void ezSceneDocument::SyncObjectHiddenState(ezDocumentObject* pObject)
{
  const bool bHidden = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid())->m_bHidden;
  m_DocumentObjectMetaData->EndReadMetaData();

  ezObjectTagMsgToEngine msg;
  msg.m_bSetTag = bHidden;
  msg.m_sTag = "EditorHidden";

  SendObjectMsg(pObject, &msg);

  for (auto pChild : pObject->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}

void ezSceneDocument::UpdateObjectDebugTargets()
{
  ezGatherObjectsForDebugVisMsgInterDoc msg;
  BroadcastInterDocumentMessage(&msg, this);

  {
    ezObjectsForDebugVisMsgToEngine msgToEngine;
    msgToEngine.m_Objects.SetCountUninitialized(sizeof(ezUuid) * msg.m_Objects.GetCount());

    ezMemoryUtils::Copy<ezUInt8>(msgToEngine.m_Objects.GetData(), reinterpret_cast<ezUInt8*>(msg.m_Objects.GetData()), msgToEngine.m_Objects.GetCount());

    GetEditorEngineConnection()->SendMessage(&msgToEngine);
  }
}
