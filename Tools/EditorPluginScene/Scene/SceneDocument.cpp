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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneObjectMetaData, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    //EZ_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
    EZ_MEMBER_PROPERTY("MetaFromPrefab", m_CreateFromPrefab),
    EZ_MEMBER_PROPERTY("MetaPrefabSeed", m_PrefabSeedGuid),
    EZ_MEMBER_PROPERTY("MetaBasePrefab", m_sBasePrefab),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSceneDocument::ezSceneDocument(const char* szDocumentPath, bool bIsPrefab) : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezSceneObjectManager))
{
  m_ActiveGizmo = ActiveGizmo::None;
  m_bIsPrefab = bIsPrefab;
  m_GameMode = GameMode::Off;
  m_fSimulationSpeed = 1.0f;
  m_bGizmoWorldSpace = true;
  m_bRenderSelectionOverlay = true;
  m_bRenderShapeIcons = true;
}

void ezSceneDocument::InitializeAfterLoading()
{
  ezDocument::InitializeAfterLoading();

  GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectPropertyEventHandler, this));
  GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectStructureEventHandler, this));
  GetObjectManager()->m_ObjectEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectEventHandler, this));

  ezEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocument::EngineConnectionEventHandler, this));
}

ezSceneDocument::~ezSceneDocument()
{
  GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectPropertyEventHandler, this));
  GetObjectManager()->m_ObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectEventHandler, this));

  ezEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::EngineConnectionEventHandler, this));
}

void ezSceneDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph)
{
  m_ObjectMetaData.AttachMetaDataToAbstractGraph(graph);
}

void ezSceneDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph)
{
  m_ObjectMetaData.RestoreMetaDataFromAbstractGraph(graph);
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
  SetGlobalTransform(pGroupObject, ezTransform(vCenter));

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

  ezAbstractGraphJsonSerializer::Write(memoryWriter, &graph, ezJSONWriter::WhitespaceMode::LessIndentation);
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

  ezAbstractGraphJsonSerializer::Write(memoryWriter, &graph, ezJSONWriter::WhitespaceMode::LessIndentation);
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
      if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
        return;

      auto pMeta = m_ObjectMetaData.BeginModifyMetaData(pObj->GetGuid());
      if (pMeta->m_bHidden != bHide)
      {
        pMeta->m_bHidden = bHide;
        m_ObjectMetaData.EndModifyMetaData(ezSceneObjectMetaData::HiddenFlag);
      }
      else
        m_ObjectMetaData.EndModifyMetaData(0);
    });

  }
}

void ezSceneDocument::HideUnselectedObjects()
{
  ShowOrHideAllObjects(ShowOrHide::Hide);

  ShowOrHideSelectedObjects(ShowOrHide::Show);
}

ezString ezSceneDocument::ReadDocumentAsString(const char* szFile) const
{
  ezFileReader file;
  if (file.Open(szFile) == EZ_FAILURE)
  {
    ezLog::Error("Failed to open document file '%s'", szFile);
    return ezString();
  }

  ezStringBuilder sGraph;
  sGraph.ReadAll(file);

  return sGraph;
}

void ezSceneDocument::SetGameMode(GameMode mode)
{
  if (m_GameMode == mode)
    return;

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
  if (m_bRenderSelectionOverlay == b)
    return;

  m_bRenderSelectionOverlay = b;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::RenderSelectionOverlayChanged;
  m_SceneEvents.Broadcast(e);
}


void ezSceneDocument::SetRenderShapeIcons(bool b)
{
  if (m_bRenderShapeIcons == b)
    return;

  m_bRenderShapeIcons = b;

  ezSceneDocumentEvent e;
  e.m_Type = ezSceneDocumentEvent::Type::RenderShapeIconsChanged;
  m_SceneEvents.Broadcast(e);
}

const ezString& ezSceneDocument::GetCachedPrefabGraph(const ezUuid& AssetGuid)
{
  if (!m_CachedPrefabGraphs.Contains(AssetGuid))
  {
    auto* pAssetInfo = ezAssetCurator::GetSingleton()->GetAssetInfo(AssetGuid);

    const auto& sPrefabFile = pAssetInfo->m_sAbsolutePath;

    m_CachedPrefabGraphs[AssetGuid] = ReadDocumentAsString(sPrefabFile);
  }

  return m_CachedPrefabGraphs[AssetGuid];
}



void ezSceneDocument::ShowOrHideAllObjects(ShowOrHide action)
{
  const bool bHide = action == ShowOrHide::Hide;

  ApplyRecursive(GetObjectManager()->GetRootObject(), [this, bHide](const ezDocumentObject* pObj)
  {
    if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
      return;

    ezUInt32 uiFlags = 0;

    auto pMeta = m_ObjectMetaData.BeginModifyMetaData(pObj->GetGuid());

    if (pMeta->m_bHidden != bHide)
    {
      pMeta->m_bHidden = bHide;
      uiFlags = ezSceneObjectMetaData::HiddenFlag;
    }

    m_ObjectMetaData.EndModifyMetaData(uiFlags);
  });
}

void ezSceneDocument::RevertPrefabs(const ezDeque<const ezDocumentObject*>& Selection)
{
  if (Selection.IsEmpty())
    return;

  auto pHistory = GetCommandHistory();

  m_CachedPrefabGraphs.Clear();

  pHistory->StartTransaction();

  for (auto pItem : Selection)
  {
    auto pMeta = m_ObjectMetaData.BeginReadMetaData(pItem->GetGuid());

    const ezUuid PrefabAsset = pMeta->m_CreateFromPrefab;

    if (!PrefabAsset.IsValid())
    {
      m_ObjectMetaData.EndReadMetaData();
      continue;
    }

    const ezVec3 vLocalPos = pItem->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>();
    const ezQuat vLocalRot = pItem->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>();
    const ezVec3 vLocalScale = pItem->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();
    const float fLocalUniformScale = pItem->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

    ezRemoveObjectCommand remCmd;
    remCmd.m_Object = pItem->GetGuid();

    ezInstantiatePrefabCommand instCmd;
    instCmd.m_bAllowPickedPosition = false;
    instCmd.m_Parent = pItem->GetParent() == GetObjectManager()->GetRootObject() ? ezUuid() : pItem->GetParent()->GetGuid();
    instCmd.m_RemapGuid = pMeta->m_PrefabSeedGuid;
    instCmd.m_sJsonGraph = GetCachedPrefabGraph(pMeta->m_CreateFromPrefab);

    ezUuid NewObject;
    void* pArray = &NewObject;
    memcpy(&instCmd.m_pCreatedRootObject, &pArray, sizeof(void*)); /// \todo HACK-o-rama

    m_ObjectMetaData.EndReadMetaData();

    pHistory->AddCommand(remCmd);
    pHistory->AddCommand(instCmd);

    if (NewObject.IsValid())
    {
      ezSetObjectPropertyCommand setCmd;
      setCmd.m_Object = NewObject;

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

      auto pMeta = m_ObjectMetaData.BeginModifyMetaData(NewObject);
      pMeta->m_CreateFromPrefab = PrefabAsset;
      pMeta->m_PrefabSeedGuid = instCmd.m_RemapGuid;
      pMeta->m_sBasePrefab = instCmd.m_sJsonGraph;

      m_ObjectMetaData.EndModifyMetaData(ezSceneObjectMetaData::PrefabFlag);
    }
  }

  pHistory->FinishTransaction();
}


void ezSceneDocument::UnlinkPrefabs(const ezDeque<const ezDocumentObject*>& Selection)
{
  if (Selection.IsEmpty())
    return;

  /// \todo this operation is (currently) not undo-able, since it only operates on meta data

  for (auto pObject : Selection)
  {
    auto pMeta = m_ObjectMetaData.BeginModifyMetaData(pObject->GetGuid());

    pMeta->m_CreateFromPrefab = ezUuid();
    pMeta->m_CachedNodeName.Clear();
    pMeta->m_PrefabSeedGuid = ezUuid();
    pMeta->m_sBasePrefab.Clear();

    m_ObjectMetaData.EndModifyMetaData(ezSceneObjectMetaData::PrefabFlag | ezSceneObjectMetaData::CachedName);
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

  m_ObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);

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

  m_ObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);

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

      SetGlobalTransform(e.m_pObject, t);
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
      m_ObjectMetaData.ClearMetaData(e.m_pObject->GetGuid());
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

void ezSceneDocument::SetGlobalTransform(const ezDocumentObject* pObject, const ezTransform& t)
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

  if (pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>() != vLocalPos)
  {
    cmd.SetPropertyPath("LocalPosition");
    cmd.m_NewValue = vLocalPos;
    pHistory->AddCommand(cmd);
  }

  if (pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>() != qLocalRot)
  {
    cmd.SetPropertyPath("LocalRotation");
    cmd.m_NewValue = qLocalRot;
    pHistory->AddCommand(cmd);
  }

  if (pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>() != vLocalScale)
  {
    cmd.SetPropertyPath("LocalScaling");
    cmd.m_NewValue = vLocalScale;
    pHistory->AddCommand(cmd);
  }

  if (pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>() != fUniformScale)
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

