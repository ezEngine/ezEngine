#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Dialogs/DuplicateDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Core/World/GameObject.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Commands/SceneCommands.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <Core/World/GameObject.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Serialization/DdlSerializer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneObjectMetaData, 1, ezRTTINoAllocator)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //  //EZ_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
  //}
  //EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocument, 2, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSceneDocument::ezSceneDocument(const char* szDocumentPath, bool bIsPrefab)
  : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezSceneObjectManager), true, true)
{
  m_ActiveGizmo = ActiveGizmo::None;
  m_bIsPrefab = bIsPrefab;
  m_GameMode = GameMode::Off;
  m_fSimulationSpeed = 1.0f;
  m_bGizmoWorldSpace = true;
  m_iResendSelection = 0;

  m_CurrentMode.m_bRenderSelectionOverlay = true;
  m_CurrentMode.m_bRenderShapeIcons = true;
  m_CurrentMode.m_bRenderVisualizers = true;
  m_CurrentMode.m_bAddAmbientLight = m_bIsPrefab;

  m_GameModeData[GameMode::Off].m_bRenderSelectionOverlay = true;
  m_GameModeData[GameMode::Off].m_bRenderShapeIcons = true;
  m_GameModeData[GameMode::Off].m_bRenderVisualizers = true;
  m_GameModeData[GameMode::Off].m_bAddAmbientLight = m_bIsPrefab;

  m_GameModeData[GameMode::Simulate].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Simulate].m_bRenderShapeIcons = false;
  m_GameModeData[GameMode::Simulate].m_bRenderVisualizers = false;
  m_GameModeData[GameMode::Simulate].m_bAddAmbientLight = m_bIsPrefab;

  m_GameModeData[GameMode::Play].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Play].m_bRenderShapeIcons = false;
  m_GameModeData[GameMode::Play].m_bRenderVisualizers = false;
  m_GameModeData[GameMode::Play].m_bAddAmbientLight = m_bIsPrefab;
}

void ezSceneDocument::InitializeAfterLoading()
{
  ezAssetDocument::InitializeAfterLoading();

  GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectPropertyEventHandler, this));
  GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectStructureEventHandler, this));
  GetObjectManager()->m_ObjectEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectEventHandler, this));
  GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocument::SelectionManagerEventHandler, this));
  m_DocumentObjectMetaData.m_DataModifiedEvent.AddEventHandler(ezMakeDelegate(&ezSceneDocument::DocumentObjectMetaDataEventHandler, this));

  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ToolsProjectEventHandler, this));

  ezEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocument::EngineConnectionEventHandler, this));
}

ezSceneDocument::~ezSceneDocument()
{
  GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectPropertyEventHandler, this));
  GetObjectManager()->m_ObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectEventHandler, this));
  GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::SelectionManagerEventHandler, this));
  m_DocumentObjectMetaData.m_DataModifiedEvent.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::DocumentObjectMetaDataEventHandler, this));

  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ToolsProjectEventHandler, this));

  ezEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::EngineConnectionEventHandler, this));
}


const char* ezSceneDocument::GetDocumentTypeDisplayString() const
{
  if (m_bIsPrefab)
    return "Prefab";

  return "Scene";
}

void ezSceneDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  ezAssetDocument::AttachMetaDataBeforeSaving(graph);

  m_SceneObjectMetaData.AttachMetaDataToAbstractGraph(graph);
}

void ezSceneDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph)
{
  ezAssetDocument::RestoreMetaDataAfterLoading(graph);

  m_SceneObjectMetaData.RestoreMetaDataFromAbstractGraph(graph);
}

void ezSceneDocument::SetActiveGizmo(ActiveGizmo gizmo) const
{
  if (m_ActiveGizmo == gizmo)
    return;

  m_ActiveGizmo = gizmo;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::ActiveGizmoChanged;
  m_SceneEvents.Broadcast(e);
}

ActiveGizmo ezSceneDocument::GetActiveGizmo() const
{
  return m_ActiveGizmo;
}

void ezSceneDocument::TriggerShowSelectionInScenegraph() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::ShowSelectionInScenegraph;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::TriggerFocusOnSelection(bool bAllViews) const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  ezSceneDocumentEvent e;
  e.m_Type = bAllViews ? ezSceneDocumentEvent::Type::FocusOnSelection_All : ezSceneDocumentEvent::Type::FocusOnSelection_Hovered;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::TriggerSnapPivotToGrid() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::SnapSelectionPivotToGrid;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::TriggerSnapEachObjectToGrid() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::SnapEachSelectedObjectToGrid;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::GroupSelection()
{
  const auto& sel = GetSelectionManager()->GetTopLevelSelection(ezGetStaticRTTI<ezGameObject>());
  if (sel.GetCount() <= 1)
    return;

  ezVec3 vCenter(0.0f);

  for (const auto& item : sel)
  {
    vCenter += GetGlobalTransform(item).m_vPosition;
  }

  vCenter /= sel.GetCount();
  //vCenter.SetZero();

  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Group Selection");

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_pType = ezGetStaticRTTI<ezGameObject>();
  cmdAdd.m_Index = -1;
  cmdAdd.m_sParentProperty = "Children";

  pHistory->AddCommand(cmdAdd);

  auto pGroupObject = GetObjectManager()->GetObject(cmdAdd.m_NewObjectGuid);
  SetGlobalTransform(pGroupObject, ezTransform(vCenter), TransformationChanges::Translation);

  ezMoveObjectCommand cmdMove;
  cmdMove.m_NewParent = cmdAdd.m_NewObjectGuid;
  cmdMove.m_Index = -1;
  cmdMove.m_sParentProperty = "Children";

  for (const auto& item : sel)
  {
    cmdMove.m_Object = item->GetGuid();
    pHistory->AddCommand(cmdMove);
  }

  pHistory->FinishTransaction();
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
  Copy(graph, &parents);

  ezStringBuilder temp;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", ezConversionUtils::ToString(it.Key()).GetData(), ezConversionUtils::ToString(it.Value()).GetData());
  }

  // Serialize to string
  ezMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  ezAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1); // null terminate

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
  cmd.m_RevolveStartAngle = ezAngle::Degree(dlg.s_iRevolveStartAngle);
  cmd.m_RevolveAngleStep = ezAngle::Degree(dlg.s_iRevolveAngleStep);

  auto history = GetCommandHistory();

  history->StartTransaction("Duplicate Special");

  if (history->AddCommand(cmd).m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}


void ezSceneDocument::SnapCameraToObject()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.GetCount() != 1)
    return;

  ezTransform trans;
  if (ComputeObjectTransformation(selection[0], trans).Failed())
    return;

  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr)
    return;

  const ezCamera* pCamera = &ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  const ezVec3 vForward = trans.m_Rotation * ezVec3(1, 0, 0);
  const ezVec3 vUp = trans.m_Rotation * ezVec3(0, 0, 1);

  ctxt.m_pLastHoveredViewWidget->InterpolateCameraTo(trans.m_vPosition, vForward, pCamera->GetFovOrDim(), &vUp);
}


void ezSceneDocument::SnapObjectToCamera()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr)
    return;

  const auto& camera = ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  ezTransform transform;
  transform.m_vPosition = camera.GetCenterPosition();
  transform.m_Rotation.SetColumn(0, camera.GetCenterDirForwards());
  transform.m_Rotation.SetColumn(1, camera.GetCenterDirRight());
  transform.m_Rotation.SetColumn(2, camera.GetCenterDirUp());

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


void ezSceneDocument::MoveCameraHere()
{
  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();
  const bool bCanMove = ctxt.m_pLastHoveredViewWidget != nullptr && ctxt.m_pLastPickingResult && !ctxt.m_pLastPickingResult->m_vPickedPosition.IsNaN();

  if (!bCanMove)
    return;

  const ezCamera* pCamera = &ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  const ezVec3 vCurPos = pCamera->GetCenterPosition();
  const ezVec3 vDirToPos = ctxt.m_pLastPickingResult->m_vPickedPosition - vCurPos;

  // don't move the entire distance, keep some distance to the target position
  const ezVec3 vPos = vCurPos + 0.9f * vDirToPos;
  const ezVec3 vForward = pCamera->GetCenterDirForwards();
  const ezVec3 vUp = pCamera->GetCenterDirUp();

  ctxt.m_pLastHoveredViewWidget->InterpolateCameraTo(vPos, vForward, pCamera->GetFovOrDim(), &vUp);
}


void ezSceneDocument::AttachToObject()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();
  if (ctxt.m_pLastHoveredViewWidget == nullptr || ctxt.m_pLastPickingResult == nullptr || !ctxt.m_pLastPickingResult->m_PickedObject.IsValid())
    return;

  ezMoveObjectCommand cmd;
  cmd.m_sParentProperty = "Children";
  cmd.m_NewParent = ctxt.m_pLastPickingResult->m_PickedObject;

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Attach to Object");
  {
    for (const ezDocumentObject* pObject : selection)
    {
      cmd.m_Object = pObject->GetGuid();

      pHistory->AddCommand(cmd);
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

      pHistory->AddCommand(cmd);
    }
  }
  pHistory->FinishTransaction();
}

ezStatus ezSceneDocument::CreateEmptyNode(bool bAttachToParent, bool bAtPickedPosition)
{
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
    cmdAdd.m_NewObjectGuid.CreateNewUuid();
    NewNode = cmdAdd.m_NewObjectGuid;

    auto res = history->AddCommand(cmdAdd);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }
  else
  {
    cmdAdd.m_NewObjectGuid.CreateNewUuid();
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
    ezSetObjectPropertyCommand cmdSet;
    cmdSet.m_NewValue = ctxt.m_pLastPickingResult->m_vPickedPosition;
    cmdSet.m_Object = NewNode;
    cmdSet.m_sProperty = "LocalPosition";

    auto res = history->AddCommand(cmdSet);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }

  history->FinishTransaction();

  GetSelectionManager()->SetSelection(GetObjectManager()->GetObject(NewNode));
  return ezStatus(EZ_SUCCESS);
}

void ezSceneDocument::DuplicateSelection()
{
  ezMap<ezUuid, ezUuid> parents;

  ezAbstractObjectGraph graph;
  Copy(graph, &parents);

  ezStringBuilder temp;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", ezConversionUtils::ToString(it.Key()).GetData(), ezConversionUtils::ToString(it.Value()).GetData());
  }

  // Serialize to string
  ezMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  ezAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1); // null terminate

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
      //if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
        //return;

      auto pMeta = m_DocumentObjectMetaData.BeginModifyMetaData(pObj->GetGuid());
      if (pMeta->m_bHidden != bHide)
      {
        pMeta->m_bHidden = bHide;
        m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::HiddenFlag);
      }
      else
        m_DocumentObjectMetaData.EndModifyMetaData(0);
    });

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
  SetAddAmbientLight(m_GameModeData[m_GameMode].m_bAddAmbientLight);

  if (m_GameMode == GameMode::Off)
  {
    // reset the game world
    ezEditorEngineProcessConnection::GetSingleton()->SendDocumentOpenMessage(this, true);
  }

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::GameModeChanged;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::StartSimulateWorld()
{
  if (m_GameMode != GameMode::Off)
  {
    StopGameMode();
    return;
  }

  SetGameMode(GameMode::Simulate);
}


void ezSceneDocument::TriggerGameModePlay()
{
  if (m_GameMode != GameMode::Off)
  {
    StopGameMode();
    return;
  }

  // attempt to start PTG
  // do not change state here
  {
    ezGameModeMsgToEngine msg;
    msg.m_bEnablePTG = true;
    GetEditorEngineConnection()->SendMessage(&msg);
  }
  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::TriggerGameModePlay;
  m_SceneEvents.Broadcast(e);
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
    ezSceneDocumentEvent e;
    e.m_Type = ezSceneDocumentEvent::Type::TriggerStopGameModePlay;
    m_SceneEvents.Broadcast(e);
  }

  return true;
}

void ezSceneDocument::SetSimulationSpeed(float f)
{
  if (m_fSimulationSpeed == f)
    return;

  m_fSimulationSpeed = f;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::SimulationSpeedChanged;
  m_SceneEvents.Broadcast(e);

  ShowDocumentStatus(ezFmt("Simulation Speed: {0}%%", (ezInt32)(m_fSimulationSpeed * 100.0f)));
}

void ezSceneDocument::SetRenderSelectionOverlay(bool b)
{
  if (m_CurrentMode.m_bRenderSelectionOverlay == b)
    return;

  m_CurrentMode.m_bRenderSelectionOverlay = b;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::RenderSelectionOverlayChanged;
  m_SceneEvents.Broadcast(e);
}


void ezSceneDocument::SetRenderVisualizers(bool b)
{
  if (m_CurrentMode.m_bRenderVisualizers == b)
    return;

  m_CurrentMode.m_bRenderVisualizers = b;

  ezVisualizerManager::GetSingleton()->SetVisualizersActive(this, m_CurrentMode.m_bRenderVisualizers);

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::RenderVisualizersChanged;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::SetRenderShapeIcons(bool b)
{
  if (m_CurrentMode.m_bRenderShapeIcons == b)
    return;

  m_CurrentMode.m_bRenderShapeIcons = b;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::RenderShapeIconsChanged;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::SetAddAmbientLight(bool b)
{
  if (m_CurrentMode.m_bAddAmbientLight == b)
    return;

  m_CurrentMode.m_bAddAmbientLight = b;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::AddAmbientLightChanged;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::ShowOrHideAllObjects(ShowOrHide action)
{
  const bool bHide = action == ShowOrHide::Hide;

  ApplyRecursive(GetObjectManager()->GetRootObject(), [this, bHide](const ezDocumentObject* pObj)
  {
    //if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
      //return;

    ezUInt32 uiFlags = 0;

    auto pMeta = m_DocumentObjectMetaData.BeginModifyMetaData(pObj->GetGuid());

    if (pMeta->m_bHidden != bHide)
    {
      pMeta->m_bHidden = bHide;
      uiFlags = ezDocumentObjectMetaData::HiddenFlag;
    }

    m_DocumentObjectMetaData.EndModifyMetaData(uiFlags);
  });
}
void ezSceneDocument::SetGizmoWorldSpace(bool bWorldSpace) const
{
  if (m_bGizmoWorldSpace == bWorldSpace)
    return;

  m_bGizmoWorldSpace = bWorldSpace;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::ActiveGizmoChanged;
  m_SceneEvents.Broadcast(e);
}

bool ezSceneDocument::GetGizmoWorldSpace() const
{
  if (m_ActiveGizmo == ActiveGizmo::Scale)
    return false;

  if (m_ActiveGizmo == ActiveGizmo::DragToPosition)
    return false;

  return m_bGizmoWorldSpace;
}

void ezSceneDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/ezEditor.ezAbstractGraph");
}

bool ezSceneDocument::Copy(ezAbstractObjectGraph& graph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.ezAbstractGraph";
  return Copy(graph, nullptr);
}

bool ezSceneDocument::Copy(ezAbstractObjectGraph& graph, ezMap<ezUuid, ezUuid>* out_pParents) const
{
  if (GetSelectionManager()->GetSelection().GetCount() == 0)
    return false;

  // Serialize selection to graph
  auto Selection = GetSelectionManager()->GetTopLevelSelection();

  ezDocumentObjectConverterWriter writer(&graph, GetObjectManager(), true, true);

  // TODO: objects are required to be named root but this is not enforced or obvious by the interface.
  for (auto item : Selection)
    writer.AddObjectToGraph(item, "root");

  if (out_pParents != nullptr)
  {
    out_pParents->Clear();

    for (auto item : Selection)
    {
      (*out_pParents)[item->GetGuid()] = item->GetParent()->GetGuid();
    }
  }

  AttachMetaDataBeforeSaving(graph);

  return true;
}

bool ezSceneDocument::PasteAt(const ezArrayPtr<PasteInfo>& info, const ezVec3& vPasteAt)
{
  ezVec3 vAvgPos(0.0f);

  for (const PasteInfo& pi : info)
  {
    if (pi.m_pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezGameObject>())
      return false;

    vAvgPos += pi.m_pObject->GetTypeAccessor().GetValue("LocalPosition").Get<ezVec3>();
  }

  vAvgPos /= info.GetCount();

  for (const PasteInfo& pi : info)
  {
    const ezVec3 vLocalPos = pi.m_pObject->GetTypeAccessor().GetValue("LocalPosition").Get<ezVec3>();
    pi.m_pObject->GetTypeAccessor().SetValue("LocalPosition", vLocalPos - vAvgPos + vPasteAt);

    if (pi.m_pParent == nullptr || pi.m_pParent == GetObjectManager()->GetRootObject())
    {
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", -1);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", -1);
    }
  }

  return true;
}

bool ezSceneDocument::PasteAtOrignalPosition(const ezArrayPtr<PasteInfo>& info)
{
  for (const PasteInfo& pi : info)
  {
    if (pi.m_pParent == nullptr || pi.m_pParent == GetObjectManager()->GetRootObject())
    {
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", -1);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", -1);
    }
  }

  return true;
}

bool ezSceneDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

  if (bAllowPickedPosition && ctxt.m_pLastPickingResult && ctxt.m_pLastPickingResult->m_PickedObject.IsValid())
  {
    if (!PasteAt(info, ctxt.m_pLastPickingResult->m_vPickedPosition))
      return false;
  }
  else
  {
    if (!PasteAtOrignalPosition(info))
      return false;
  }

  m_DocumentObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);
  m_SceneObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);

  // set the pasted objects as the new selection
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

bool ezSceneDocument::Duplicate(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bSetSelected)
{
  if (!PasteAtOrignalPosition(info))
    return false;

  m_DocumentObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);
  m_SceneObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);

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

void ezSceneDocument::ObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "LocalPosition" ||
      e.m_sProperty == "LocalRotation" ||
      e.m_sProperty == "LocalScaling" ||
      e.m_sProperty == "LocalUniformScaling")
  {
    InvalidateGlobalTransformValue(e.m_pObject);
  }
}

void ezSceneDocument::ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (!e.m_pObject->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    {
      // make sure the cache is filled with a proper value
      GetGlobalTransform(e.m_pObject);
    }
    break;

  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
    {
      // read cached value, hopefully it was not invalidated in between BeforeObjectMoved and AfterObjectMoved
      ezTransform t = GetGlobalTransform(e.m_pObject);

      SetGlobalTransform(e.m_pObject, t, TransformationChanges::All);
    }
    break;
  }
}


void ezSceneDocument::ObjectEventHandler(const ezDocumentObjectEvent& e)
{
  if (!e.m_pObject->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  switch (e.m_EventType)
  {
  case ezDocumentObjectEvent::Type::BeforeObjectDestroyed:
    {
      // clean up object meta data upon object destruction, because we can :-P
      if (GetObjectManager()->GetObject(e.m_pObject->GetGuid()) == nullptr)
      {
        // make sure there is no object with this GUID still "added" to the document
        // this can happen if two objects use the same GUID, only one object can be "added" at a time, but multiple objects with the same GUID may exist
        // the same GUID is in use, when a prefab is recreated (updated) and the GUIDs are restored, such that references don't change
        // the object that is being destroyed is typically referenced by a command that was in the redo-queue that got purged

        m_DocumentObjectMetaData.ClearMetaData(e.m_pObject->GetGuid());
        m_SceneObjectMetaData.ClearMetaData(e.m_pObject->GetGuid());
      }
    }
    break;
  }
}


void ezSceneDocument::SelectionManagerEventHandler(const ezSelectionManagerEvent& e)
{
  m_iResendSelection = 2;
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

void ezSceneDocument::SendObjectSelection()
{
  if (m_iResendSelection <= 0)
    return;

  --m_iResendSelection;

  const auto& sel = GetSelectionManager()->GetSelection();

  ezObjectSelectionMsgToEngine msg;
  ezStringBuilder sTemp;
  ezString sGuid;

  for (const auto& item : sel)
  {
    sGuid = ezConversionUtils::ToString(item->GetGuid());

    sTemp.Append(";", sGuid);
  }

  msg.m_sSelection = sTemp;

  GetEditorEngineConnection()->SendMessage(&msg);
}




const ezTransform& ezSceneDocument::GetGlobalTransform(const ezDocumentObject* pObject) const
{
  ezTransform Trans;

  if (!m_GlobalTransforms.TryGetValue(pObject, Trans))
  {
    Trans = ComputeGlobalTransform(pObject);
  }

  return m_GlobalTransforms[pObject];
}

void ezSceneDocument::SetGlobalTransform(const ezDocumentObject* pObject, const ezTransform& t, ezUInt8 transformationChanges) const
{
  auto pHistory = GetCommandHistory();
  if (!pHistory->IsInTransaction())
  {
    InvalidateGlobalTransformValue(pObject);
    return;
  }

  const ezDocumentObject* pParent = pObject->GetParent();

  ezTransform tLocal;

  if (pParent != nullptr)
  {
    ezTransform tParent = GetGlobalTransform(pParent);

    tLocal.SetLocalTransform(tParent, t);
  }
  else
    tLocal = t;

  ezVec3 vLocalPos;
  ezVec3 vLocalScale;
  ezQuat qLocalRot;
  float fUniformScale = 1.0f;

  tLocal.Decompose(vLocalPos, qLocalRot, vLocalScale);

  if (vLocalScale.x == vLocalScale.y && vLocalScale.x == vLocalScale.z)
  {
    fUniformScale = vLocalScale.x;
    vLocalScale.Set(1.0f);
  }

  ezSetObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();

  // unfortunately when we are dragging an object the 'temporary' transaction is undone every time before the new commands are sent
  // that means the values that we read here, are always the original values before the object was modified at all
  // therefore when the original position and the new position are identical, that means the user dragged the object to the previous position
  // it does NOT mean that there is no change, in fact there is a change, just back to the original value

  //if (pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>() != vLocalPos)
  if ((transformationChanges & TransformationChanges::Translation) != 0)
  {
    cmd.m_sProperty = "LocalPosition";
    cmd.m_NewValue = vLocalPos;
    pHistory->AddCommand(cmd);
  }

  //if (pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>() != qLocalRot)
  if ((transformationChanges & TransformationChanges::Rotation) != 0)
  {
    cmd.m_sProperty = "LocalRotation";
    cmd.m_NewValue = qLocalRot;
    pHistory->AddCommand(cmd);
  }

  //if (pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>() != vLocalScale)
  if ((transformationChanges & TransformationChanges::Scale) != 0)
  {
    cmd.m_sProperty = "LocalScaling";
    cmd.m_NewValue = vLocalScale;
    pHistory->AddCommand(cmd);
  }

  //if (pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>() != fUniformScale)
  if ((transformationChanges & TransformationChanges::UniformScale) != 0)
  {
    cmd.m_sProperty = "LocalUniformScaling";
    cmd.m_NewValue = fUniformScale;
    pHistory->AddCommand(cmd);
  }

  // will be recomputed the next time it is queried
  InvalidateGlobalTransformValue(pObject);
}

ezStatus ezSceneDocument::RequestExportScene(const char* szTargetFile, const ezAssetFileHeader& header)
{
  EZ_SUCCEED_OR_RETURN(SaveDocument());

  return ezAssetDocument::RemoteExport(header, szTargetFile);
}

void ezSceneDocument::InvalidateGlobalTransformValue(const ezDocumentObject* pObject) const
{
  // will be recomputed the next time it is queried
  m_GlobalTransforms.Remove(pObject);

  /// \todo If all parents are always inserted as well, we can stop once an object is found that is not in the list

  for (auto pChild : pObject->GetChildren())
  {
    InvalidateGlobalTransformValue(pChild);
  }
}

const char* ezSceneDocument::QueryAssetType() const
{
  if (m_bIsPrefab)
    return "Prefab";

  return "Scene";
}

ezStatus ezSceneDocument::ExportScene()
{
  auto saveres = SaveDocument();

  if (saveres.m_Result.Failed())
    return saveres;

  auto res = TransformAssetManually();

  if (res.m_Result.Failed())
    ezLog::Error(res.m_sMessage);
  else
    ezLog::Success(res.m_sMessage);

  ShowDocumentStatus(res.m_sMessage.GetData());

  return res;
}


ezResult ezSceneDocument::ComputeObjectTransformation(const ezDocumentObject* pObject, ezTransform& out_Result) const
{
  const ezDocumentObject* pObj = pObject;

  while (pObj && !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
  {
    pObj = pObj->GetParent();
  }

  if (pObj)
  {
    out_Result = ComputeGlobalTransform(pObj);
    return EZ_SUCCESS;
  }
  else
  {
    out_Result.SetIdentity();
    return EZ_FAILURE;
  }
}


void ezSceneDocument::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  ezAssetDocument::HandleEngineMessage(pMsg);

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezGameModeMsgToEditor>())
  {
    HandleGameModeMsg(static_cast<const ezGameModeMsgToEditor*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentOpenResponseMsgToEditor>())
  {
    SyncObjectHiddenState();
  }
}

ezStatus ezSceneDocument::InternalTransformAsset(const char* szTargetFile, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  return RequestExportScene(szTargetFile, AssetHeader);
}


ezStatus ezSceneDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  EZ_ASSERT_NOT_IMPLEMENTED;

  /* this function is never called */
  return ezStatus(EZ_FAILURE);
}


ezStatus ezSceneDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);

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
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}

void ezSceneDocument::SyncObjectHiddenState(ezDocumentObject* pObject)
{
  const bool bHidden = m_DocumentObjectMetaData.BeginReadMetaData(pObject->GetGuid())->m_bHidden;
  m_DocumentObjectMetaData.EndReadMetaData();

  ezObjectTagMsgToEngine msg;
  msg.m_bSetTag = bHidden;
  msg.m_sTag = "EditorHidden";

  SendObjectMsg(pObject, &msg);

  for (auto pChild : pObject->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}
