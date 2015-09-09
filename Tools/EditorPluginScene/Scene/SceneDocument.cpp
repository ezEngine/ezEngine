#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Objects/TestObjects.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Core/World/GameObject.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Commands/SceneCommands.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocument, ezDocumentBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSceneDocument::ezSceneDocument(const char* szDocumentPath) : ezDocumentBase(szDocumentPath, EZ_DEFAULT_NEW(ezSceneObjectManager))
{
  m_ActiveGizmo = ActiveGizmo::None;
}

void ezSceneDocument::InitializeAfterLoading()
{
  ezDocumentBase::InitializeAfterLoading();

  GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectPropertyEventHandler, this));
  GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectStructureEventHandler, this));
}

ezSceneDocument::~ezSceneDocument()
{
  GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectPropertyEventHandler, this));
}

ezStatus ezSceneDocument::InternalSaveDocument()
{
  return ezDocumentBase::InternalSaveDocument();
}

void ezSceneDocument::SetActiveGizmo(ActiveGizmo gizmo)
{
  if (m_ActiveGizmo == gizmo)
    return;

  m_ActiveGizmo = gizmo;

  SceneEvent e;
  e.m_Type = SceneEvent::Type::ActiveGizmoChanged;
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

  SceneEvent e;
  e.m_Type = SceneEvent::Type::ShowSelectionInScenegraph;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::TriggerFocusOnSelection()
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  SceneEvent e;
  e.m_Type = SceneEvent::Type::FocusOnSelection;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::TriggerSnapPivotToGrid()
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  SceneEvent e;
  e.m_Type = SceneEvent::Type::SnapSelectionPivotToGrid;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::TriggerSnapEachObjectToGrid()
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  SceneEvent e;
  e.m_Type = SceneEvent::Type::SnapEachSelectedObjectToGrid;
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
  cmdAdd.m_sParentProperty = "RootObjects";

  pHistory->AddCommand(cmdAdd);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.SetPropertyPath("Name");
  cmdSet.m_NewValue = "Group";
  pHistory->AddCommand(cmdSet);

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

void ezSceneDocument::TriggerHideSelectedObjects()
{
  SceneEvent e;
  e.m_Type = SceneEvent::Type::HideSelectedObjects;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::TriggerHideUnselectedObjects()
{
  SceneEvent e;
  e.m_Type = SceneEvent::Type::HideUnselectedObjects;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::TriggerShowHiddenObjects()
{
  SceneEvent e;
  e.m_Type = SceneEvent::Type::ShowHiddenObjects;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::SetGizmoWorldSpace(bool bWorldSpace)
{
  if (m_bGizmoWorldSpace == bWorldSpace)
    return;

  m_bGizmoWorldSpace = bWorldSpace;

  SceneEvent e;
  e.m_Type = SceneEvent::Type::ActiveGizmoChanged;
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
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "RootObjects", -1);
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
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "RootObjects", -1);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", -1);
    }
  }

  return true;
}

bool ezSceneDocument::Paste(const ezArrayPtr<PasteInfo>& info)
{
  if (m_PickingResult.m_PickedObject.IsValid())
  {
    if (!PasteAt(info, m_PickingResult.m_vPickedPosition))
      return false;
  }
  else
  {
    if (!PasteAtOrignalPosition(info))
      return false;
  }

  // set the pasted objects as the new selection
  {
    auto pSelMan = GetSelectionManager();

    ezDeque<const ezDocumentObjectBase*> NewSelection;

    for (const PasteInfo& pi : info)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }

  return true;
}

bool ezSceneDocument::Duplicate(const ezArrayPtr<PasteInfo>& info)
{
  if (!PasteAtOrignalPosition(info))
    return false;

  // set the pasted objects as the new selection
  {
    auto pSelMan = GetSelectionManager();

    ezDeque<const ezDocumentObjectBase*> NewSelection;

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
      e.m_sPropertyPath == "LocalScaling")
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

const ezTransform& ezSceneDocument::GetGlobalTransform(const ezDocumentObjectBase* pObject)
{
  ezTransform Trans;

  if (!m_GlobalTransforms.TryGetValue(pObject, Trans))
  {
    /// \todo Insert all parents as well

    Trans = ComputeGlobalTransform(pObject);
    m_GlobalTransforms[pObject] = Trans;
  }

  return m_GlobalTransforms[pObject];
}

void ezSceneDocument::SetGlobalTransform(const ezDocumentObjectBase* pObject, const ezTransform& t)
{
  auto pHistory = GetCommandHistory();
  if (!pHistory->IsInTransaction())
  {
    InvalidateGlobalTransformValue(pObject);
    return;
  }

  const ezDocumentObjectBase* pParent = pObject->GetParent();

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

  tLocal.Decompose(vLocalPos, qLocalRot, vLocalScale);

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

  // will be recomputed the next time it is queried
  InvalidateGlobalTransformValue(pObject);
}

void ezSceneDocument::InvalidateGlobalTransformValue(const ezDocumentObjectBase* pObject)
{
  // will be recomputed the next time it is queried
  m_GlobalTransforms.Remove(pObject);

  /// \todo If all parents are always inserted as well, we can stop once an object is found that is not in the list

  for (auto pChild : pObject->GetChildren())
  {
    InvalidateGlobalTransformValue(pChild);
  }
}