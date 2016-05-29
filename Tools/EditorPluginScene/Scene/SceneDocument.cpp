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
#include <Foundation/Serialization/JsonSerializer.h>
#include <Commands/SceneCommands.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <Core/World/GameObject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneObjectMetaData, 1, ezRTTINoAllocator)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //  //EZ_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
  //}
  //EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSceneDocument::ezSceneDocument(const char* szDocumentPath, bool bIsPrefab) : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezSceneObjectManager))
{
  m_ActiveGizmo = ActiveGizmo::None;
  m_bIsPrefab = bIsPrefab;
  m_GameMode = GameMode::Off;
  m_fSimulationSpeed = 1.0f;
  m_bGizmoWorldSpace = true;

  m_CurrentMode.m_bRenderSelectionOverlay = true;
  m_CurrentMode.m_bRenderShapeIcons = true;
  m_CurrentMode.m_bRenderVisualizers = true;

  m_GameModeData[GameMode::Off].m_bRenderSelectionOverlay = true;
  m_GameModeData[GameMode::Off].m_bRenderShapeIcons = true;
  m_GameModeData[GameMode::Off].m_bRenderVisualizers = true;

  m_GameModeData[GameMode::Simulate].m_bRenderSelectionOverlay = true;
  m_GameModeData[GameMode::Simulate].m_bRenderShapeIcons = false;
  m_GameModeData[GameMode::Simulate].m_bRenderVisualizers = false;

  m_GameModeData[GameMode::Play].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Play].m_bRenderShapeIcons = false;
  m_GameModeData[GameMode::Play].m_bRenderVisualizers = false;
}

void ezSceneDocument::InitializeAfterLoading()
{
  ezDocument::InitializeAfterLoading();

  GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectPropertyEventHandler, this));
  GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectStructureEventHandler, this));
  GetObjectManager()->m_ObjectEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ToolsProjectEventHandler, this));

  ezEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocument::EngineConnectionEventHandler, this));
}

ezSceneDocument::~ezSceneDocument()
{
  GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectPropertyEventHandler, this));
  GetObjectManager()->m_ObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectEventHandler, this));
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ToolsProjectEventHandler, this));

  ezEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::EngineConnectionEventHandler, this));
}

void ezSceneDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph)
{
  ezAssetDocument::AttachMetaDataBeforeSaving(graph);

  m_SceneObjectMetaData.AttachMetaDataToAbstractGraph(graph);
}

void ezSceneDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph)
{
  ezAssetDocument::RestoreMetaDataAfterLoading(graph);

  m_SceneObjectMetaData.RestoreMetaDataFromAbstractGraph(graph);
}

void ezSceneDocument::SetActiveGizmo(ActiveGizmo gizmo)
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

void ezSceneDocument::TriggerShowSelectionInScenegraph()
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::ShowSelectionInScenegraph;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::TriggerFocusOnSelection(bool bAllViews)
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  ezSceneDocumentEvent e;
  e.m_Type = bAllViews ? ezSceneDocumentEvent::Type::FocusOnSelection_All : ezSceneDocumentEvent::Type::FocusOnSelection_Hovered;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::TriggerSnapPivotToGrid()
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::SnapSelectionPivotToGrid;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::TriggerSnapEachObjectToGrid()
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

  pHistory->StartTransaction();

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_pType = ezGetStaticRTTI<ezGameObject>();
  cmdAdd.m_Index = -1;
  cmdAdd.m_sParentProperty = "Children";

  pHistory->AddCommand(cmdAdd);

  auto pGroupObject = GetObjectManager()->GetObject(cmdAdd.m_NewObjectGuid);
  SetGlobalTransform(pGroupObject, ezTransform(vCenter), ezSceneDocument::Translation);

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

  qtDuplicateDlg dlg(nullptr);
  if (dlg.exec() == QDialog::Rejected)
    return;

  ezMap<ezUuid, ezUuid> parents;

  ezAbstractObjectGraph graph;
  Copy(graph, &parents);

  ezStringBuilder temp;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("%s=%s;", ezConversionUtils::ToString(it.Key()).GetData(), ezConversionUtils::ToString(it.Value()).GetData());
  }

  // Serialize to string
  ezMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  ezAbstractGraphJsonSerializer::Write(memoryWriter, &graph, nullptr, ezJSONWriter::WhitespaceMode::LessIndentation);
  memoryWriter.WriteBytes("\0", 1); // null terminate

  ezDuplicateObjectsCommand cmd;
  cmd.m_sJsonGraph = (const char*)streamStorage.GetData();
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

  history->StartTransaction();

  if (history->AddCommand(cmd).m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}

void ezSceneDocument::CreateEmptyNode()
{
  auto history = GetCommandHistory();

  history->StartTransaction();

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_pType = ezGetStaticRTTI<ezGameObject>();
  cmdAdd.m_sParentProperty = "Children";
  cmdAdd.m_Index = -1;

  const auto& Sel = GetSelectionManager()->GetSelection();

  if (Sel.IsEmpty())
  {
    history->AddCommand(cmdAdd);
  }
  else
  {
    for (auto pParent : Sel)
    {
      cmdAdd.m_Parent = pParent->GetGuid();
      history->AddCommand(cmdAdd);
    }
  }

  history->FinishTransaction();
}

void ezSceneDocument::DuplicateSelection()
{
  ezMap<ezUuid, ezUuid> parents;

  ezAbstractObjectGraph graph;
  Copy(graph, &parents);

  ezStringBuilder temp;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("%s=%s;", ezConversionUtils::ToString(it.Key()).GetData(), ezConversionUtils::ToString(it.Value()).GetData());
  }

  // Serialize to string
  ezMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  ezAbstractGraphJsonSerializer::Write(memoryWriter, &graph, nullptr, ezJSONWriter::WhitespaceMode::LessIndentation);
  memoryWriter.WriteBytes("\0", 1); // null terminate

  ezDuplicateObjectsCommand cmd;
  cmd.m_sJsonGraph = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes = temp;

  auto history = GetCommandHistory();

  history->StartTransaction();

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

void ezSceneDocument::SetGameMode(GameMode mode)
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

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::TriggerGameModePlay;
  m_SceneEvents.Broadcast(e);
}


void ezSceneDocument::StopGameMode()
{
  if (m_GameMode == GameMode::Off)
    return;

  if (m_GameMode == GameMode::Simulate)
  {
    // we can set that state immediately
    SetGameMode(GameMode::Off);
  }

  if (m_GameMode == GameMode::Play)
  {
    // attempt to stop PTG
    // do not change any state, that will be done by the response msg

    ezSceneDocumentEvent e;
    e.m_Type = ezSceneDocumentEvent::Type::TriggerStopGameModePlay;
    m_SceneEvents.Broadcast(e);
  }
}

void ezSceneDocument::SetSimulationSpeed(float f)
{
  if (m_fSimulationSpeed == f)
    return;

  m_fSimulationSpeed = f;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::SimulationSpeedChanged;
  m_SceneEvents.Broadcast(e);

  ShowDocumentStatus("Simulation Speed: %i%%", (ezInt32)(m_fSimulationSpeed * 100.0f));
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

void ezSceneDocument::UnlinkPrefabs(const ezDeque<const ezDocumentObject*>& Selection)
{
  SUPER::UnlinkPrefabs(Selection);

  // Clear cached names.
  for (auto pObject : Selection)
  {
    auto pMetaScene = m_SceneObjectMetaData.BeginModifyMetaData(pObject->GetGuid());
    pMetaScene->m_CachedNodeName.Clear();
    m_SceneObjectMetaData.EndModifyMetaData(ezSceneObjectMetaData::CachedName);
  }
}

void ezSceneDocument::SetGizmoWorldSpace(bool bWorldSpace)
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

bool ezSceneDocument::Copy(ezAbstractObjectGraph& graph)
{
  return Copy(graph, nullptr);
}

bool ezSceneDocument::Copy(ezAbstractObjectGraph& graph, ezMap<ezUuid, ezUuid>* out_pParents)
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


void ezSceneDocument::UpdatePrefabs()
{
  EZ_LOCK(m_SceneObjectMetaData.GetMutex());
  SUPER::UpdatePrefabs();
}


ezUuid ezSceneDocument::ReplaceByPrefab(const ezDocumentObject* pRootObject, const char* szPrefabFile, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed)
{
  ezUuid newGuid = SUPER::ReplaceByPrefab(pRootObject, szPrefabFile, PrefabAsset, PrefabSeed);
  if (newGuid.IsValid())
  {
    auto pMeta = m_SceneObjectMetaData.BeginModifyMetaData(newGuid);
    pMeta->m_CachedNodeName.Clear();
    m_SceneObjectMetaData.EndModifyMetaData(ezSceneObjectMetaData::CachedName);
  }
  return newGuid;
}


ezUuid ezSceneDocument::RevertPrefab(const ezDocumentObject* pObject)
{
  auto pHistory = GetCommandHistory();
  const ezVec3 vLocalPos = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>();
  const ezQuat vLocalRot = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>();
  const ezVec3 vLocalScale = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();
  const float fLocalUniformScale = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  ezUuid newGuid = SUPER::RevertPrefab(pObject);

  if (newGuid.IsValid())
  {
    ezSetObjectPropertyCommand setCmd;
    setCmd.m_Object = newGuid;

    setCmd.m_sPropertyPath = "LocalPosition";
    setCmd.m_NewValue = vLocalPos;
    pHistory->AddCommand(setCmd);

    setCmd.m_sPropertyPath = "LocalRotation";
    setCmd.m_NewValue = vLocalRot;
    pHistory->AddCommand(setCmd);

    setCmd.m_sPropertyPath = "LocalScaling";
    setCmd.m_NewValue = vLocalScale;
    pHistory->AddCommand(setCmd);

    setCmd.m_sPropertyPath = "LocalUniformScaling";
    setCmd.m_NewValue = fLocalUniformScale;
    pHistory->AddCommand(setCmd);
  }
  return newGuid;
}

bool ezSceneDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition)
{
  if (bAllowPickedPosition && m_PickingResult.m_PickedObject.IsValid())
  {
    if (!PasteAt(info, m_PickingResult.m_vPickedPosition))
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
  if (e.m_sPropertyPath == "LocalPosition" ||
      e.m_sPropertyPath == "LocalRotation" ||
      e.m_sPropertyPath == "LocalScaling" ||
      e.m_sPropertyPath == "LocalUniformScaling")
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

      SetGlobalTransform(e.m_pObject, t, ezSceneDocument::All);
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
      m_DocumentObjectMetaData.ClearMetaData(e.m_pObject->GetGuid());
      m_SceneObjectMetaData.ClearMetaData(e.m_pObject->GetGuid());
    }
    break;
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


void ezSceneDocument::ToolsProjectEventHandler(const ezToolsProject::Event& e)
{
  switch (e.m_Type)
  {
  case ezToolsProject::Event::Type::ProjectConfigChanged:
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
const ezTransform& ezSceneDocument::GetGlobalTransform(const ezDocumentObject* pObject)
{
  ezTransform Trans;

  if (!m_GlobalTransforms.TryGetValue(pObject, Trans))
  {
    Trans = ComputeGlobalTransform(pObject);
  }

  return m_GlobalTransforms[pObject];
}

void ezSceneDocument::SetGlobalTransform(const ezDocumentObject* pObject, const ezTransform& t, ezUInt8 transformationChanges)
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
    cmd.SetPropertyPath("LocalPosition");
    cmd.m_NewValue = vLocalPos;
    pHistory->AddCommand(cmd);
  }

  //if (pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>() != qLocalRot)
  if ((transformationChanges & TransformationChanges::Rotation) != 0)
  {
    cmd.SetPropertyPath("LocalRotation");
    cmd.m_NewValue = qLocalRot;
    pHistory->AddCommand(cmd);
  }

  //if (pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>() != vLocalScale)
  if ((transformationChanges & TransformationChanges::Scale) != 0)
  {
    cmd.SetPropertyPath("LocalScaling");
    cmd.m_NewValue = vLocalScale;
    pHistory->AddCommand(cmd);
  }

  //if (pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>() != fUniformScale)
  if ((transformationChanges & TransformationChanges::UniformScale) != 0)
  {
    cmd.SetPropertyPath("LocalUniformScaling");
    cmd.m_NewValue = fUniformScale;
    pHistory->AddCommand(cmd);
  }

  // will be recomputed the next time it is queried
  InvalidateGlobalTransformValue(pObject);
}

void ezSceneDocument::InvalidateGlobalTransformValue(const ezDocumentObject* pObject)
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

void ezSceneDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo)
{
  /// \todo go through all objects, get asset properties, retrieve dependencies


}


ezStatus ezSceneDocument::ExportScene()
{
  auto res = TransformAssetManually();

  if (res.m_Result.Failed())
    ezLog::Error(res.m_sMessage);
  else
    ezLog::Success(res.m_sMessage);

  ShowDocumentStatus(res.m_sMessage);

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

ezStatus ezSceneDocument::InternalTransformAsset(const char* szTargetFile, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  ezSceneDocumentExportEvent e;
  e.m_szTargetFile = szTargetFile;
  e.m_pAssetFileHeader = &AssetHeader;
  e.m_ReturnStatus = ezStatus("No export handler available");

  m_ExportEvent.Broadcast(e);

  return e.m_ReturnStatus;
}


ezStatus ezSceneDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform)
{
  EZ_ASSERT_NOT_IMPLEMENTED;

  /* this function is never called */
  return ezStatus(EZ_FAILURE);
}

ezStatus ezSceneDocument::InternalRetrieveAssetInfo(const char* szPlatform)
{
  return ezStatus(EZ_SUCCESS);
}

ezUInt16 ezSceneDocument::GetAssetTypeVersion() const
{
  return 1;
}

ezBitflags<ezAssetDocumentFlags> ezSceneDocument::GetAssetFlags() const
{
  if (IsPrefab())
  {
    return ezAssetDocumentFlags::AutoTransformOnSave | ezAssetDocumentFlags::TransformRequiresWindow;
  }
  else
  {
    return ezAssetDocumentFlags::OnlyTransformManually | ezAssetDocumentFlags::TransformRequiresWindow;
  }
}

