#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <ToolsFoundation/Command/TreeCommands.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezComponentDragDropHandler, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezComponentDragDropHandler::CreateDropObject(const ezVec3& vPosition, const char* szType, const char* szProperty, const ezVariant& value, ezUuid parent, ezInt32 iInsertChildIndex)
{
  ezVec3 vPos = vPosition;

  if (vPos.IsNaN())
    vPos.SetZero();

  ezUuid ObjectGuid = ezUuid::MakeUuid();

  ezAddObjectCommand cmd;
  cmd.m_Parent = parent;
  cmd.m_Index = iInsertChildIndex;
  cmd.SetType("ezGameObject");
  cmd.m_NewObjectGuid = ObjectGuid;
  cmd.m_sParentProperty = "Children";

  auto history = m_pDocument->GetCommandHistory();

  EZ_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_Object = ObjectGuid;

  cmd2.m_sProperty = "LocalPosition";
  cmd2.m_NewValue = vPos;
  EZ_VERIFY(history->AddCommand(cmd2).Succeeded(), "AddCommand failed");

  AttachComponentToObject(szType, szProperty, value, ObjectGuid);

  m_DraggedObjects.PushBack(ObjectGuid);
}

void ezComponentDragDropHandler::AttachComponentToObject(const char* szType, const char* szProperty, const ezVariant& value, ezUuid ObjectGuid)
{
  auto history = m_pDocument->GetCommandHistory();

  ezUuid CmpGuid = ezUuid::MakeUuid();

  ezAddObjectCommand cmd;

  cmd.SetType(szType);
  cmd.m_sParentProperty = "Components";
  cmd.m_Index = -1;
  cmd.m_NewObjectGuid = CmpGuid;
  cmd.m_Parent = ObjectGuid;
  EZ_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

  if (value.IsA<ezVariantArray>())
  {
    ezResizeAndSetObjectPropertyCommand cmd2;
    cmd2.m_Object = CmpGuid;
    cmd2.m_sProperty = szProperty;
    cmd2.m_NewValue = value.Get<ezVariantArray>()[0];
    cmd2.m_Index = 0;
    EZ_VERIFY(history->AddCommand(cmd2).Succeeded(), "AddCommand failed");
  }
  else
  {
    ezSetObjectPropertyCommand cmd2;
    cmd2.m_Object = CmpGuid;
    cmd2.m_sProperty = szProperty;
    cmd2.m_NewValue = value;
    EZ_VERIFY(history->AddCommand(cmd2).Succeeded(), "AddCommand failed");
  }
}

void ezComponentDragDropHandler::MoveObjectToPosition(const ezUuid& guid, const ezVec3& vPosition, const ezQuat& qRotation)
{
  auto history = m_pDocument->GetCommandHistory();

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_Object = guid;

  cmd2.m_sProperty = "LocalPosition";
  cmd2.m_NewValue = vPosition;
  history->AddCommand(cmd2).AssertSuccess();

  if (qRotation.IsValid())
  {
    cmd2.m_sProperty = "LocalRotation";
    cmd2.m_NewValue = qRotation;
    history->AddCommand(cmd2).AssertSuccess();
  }
}

void ezComponentDragDropHandler::MoveDraggedObjectsToPosition(ezVec3 vPosition, bool bAllowSnap, const ezVec3& normal)
{
  if (m_DraggedObjects.IsEmpty() || !vPosition.IsValid())
    return;

  if (bAllowSnap)
  {
    ezSnapProvider::SnapTranslation(vPosition);
  }

  auto history = m_pDocument->GetCommandHistory();

  ezGameObjectDocument* pGameDoc = ezDynamicCast<ezGameObjectDocument*>(m_pDocument);

  history->StartTransaction("Move to Position");

  ezQuat rot;
  rot.SetIdentity();

  if (normal.IsValid() && !m_vAlignAxisWithNormal.IsZero(0.01f))
  {
    rot = ezQuat::MakeShortestRotation(m_vAlignAxisWithNormal, normal);
  }

  for (const auto& guid : m_DraggedObjects)
  {
    ezVec3 vNewPos = vPosition;
    ezQuat qNewRot = rot;

    if (pGameDoc)
    {
      const ezDocumentObject* pObject = m_pDocument->GetObjectManager()->GetObject(guid);
      if (const ezDocumentObject* pParent = pObject->GetParent())
      {
        const ezTransform tParent = pGameDoc->GetGlobalTransform(pParent);
        const ezTransform rRel = ezTransform::MakeLocalTransform(tParent, ezTransform(vNewPos, qNewRot));

        vNewPos = rRel.m_vPosition;
        qNewRot = rRel.m_qRotation;
      }
    }

    MoveObjectToPosition(guid, vNewPos, qNewRot);
  }

  history->FinishTransaction();
}

void ezComponentDragDropHandler::SelectCreatedObjects()
{
  ezDeque<const ezDocumentObject*> NewSel;
  for (const auto& id : m_DraggedObjects)
  {
    NewSel.PushBack(m_pDocument->GetObjectManager()->GetObject(id));
  }

  if (m_bSelectionAsRuntimeOverride)
  {
    m_pDocument->GetSelectionManager()->SetRuntimeOverrideSelection(NewSel);
  }
  else
  {
    m_pDocument->GetSelectionManager()->SetRuntimeOverrideSelection({});
    m_pDocument->GetSelectionManager()->SetSelection(NewSel);
  }
}

void ezComponentDragDropHandler::BeginTemporaryCommands()
{
  m_pDocument->GetCommandHistory()->BeginTemporaryCommands("Adjust Objects");
}

void ezComponentDragDropHandler::EndTemporaryCommands()
{
  m_pDocument->GetCommandHistory()->FinishTemporaryCommands();
}

void ezComponentDragDropHandler::CancelTemporaryCommands()
{
  if (m_DraggedObjects.IsEmpty())
    return;

  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}

void ezComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  m_pDocument = ezDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);
  EZ_ASSERT_DEV(m_pDocument != nullptr, "Invalid document GUID in drag & drop operation");

  m_pDocument->GetCommandHistory()->StartTransaction("Drag Object");
}

void ezComponentDragDropHandler::OnDragUpdate(const ezDragDropInfo* pInfo)
{
  ezVec3 vPos = pInfo->m_vDropPosition;

  if (vPos.IsNaN() || !pInfo->m_TargetObject.IsValid())
    vPos.SetZero();

  ezVec3 vNormal = pInfo->m_vDropNormal;

  if (!vNormal.IsValid() || vNormal.IsZero())
    vNormal = ezVec3(1, 0, 0);

  MoveDraggedObjectsToPosition(vPos, !pInfo->m_bShiftKeyDown, vNormal);
}

void ezComponentDragDropHandler::OnDragCancel()
{
  CancelTemporaryCommands();
  m_pDocument->GetCommandHistory()->CancelTransaction();

  m_DraggedObjects.Clear();

  m_pDocument->GetSelectionManager()->SetRuntimeOverrideSelection({});
}

void ezComponentDragDropHandler::OnDrop(const ezDragDropInfo* pInfo)
{
  EndTemporaryCommands();
  m_pDocument->GetCommandHistory()->FinishTransaction();

  m_bSelectionAsRuntimeOverride = false;
  SelectCreatedObjects();

  m_DraggedObjects.Clear();
}

float ezComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext != "viewport" && pInfo->m_sTargetContext != "scenetree")
    return 0.0f;

  const ezDocument* pDocument = ezDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);

  const ezRTTI* pRttiScene = ezRTTI::FindTypeByName("ezSceneDocument");

  if (pRttiScene == nullptr)
    return 0.0f;

  if (!pDocument->GetDynamicRTTI()->IsDerivedFrom(pRttiScene))
    return 0.0f;

  return 1.0f;
}
