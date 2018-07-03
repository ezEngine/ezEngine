#include <PCH.h>
#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorFramework/Gizmos/SnapProvider.h>

#include <QMimeData>
#include <QDataStream>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Document/DocumentManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezComponentDragDropHandler, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezComponentDragDropHandler::CreateDropObject(const ezVec3& vPosition, const char* szType, const char* szProperty, const char* szValue, ezUuid parent, ezInt32 iInsertChildIndex)
{
  ezVec3 vPos = vPosition;

  if (vPos.IsNaN())
    vPos.SetZero();

  ezUuid ObjectGuid;
  ObjectGuid.CreateNewUuid();

  ezAddObjectCommand cmd;
  cmd.m_Parent = parent;
  cmd.m_Index = iInsertChildIndex;
  cmd.SetType("ezGameObject");
  cmd.m_NewObjectGuid = ObjectGuid;
  cmd.m_sParentProperty = "Children";

  auto history = m_pDocument->GetCommandHistory();

  EZ_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_Object = ObjectGuid;

  cmd2.m_sProperty = "LocalPosition";
  cmd2.m_NewValue = vPos;
  EZ_VERIFY(history->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");

  AttachComponentToObject(szType, szProperty, szValue, ObjectGuid);

  m_DraggedObjects.PushBack(ObjectGuid);
}

void ezComponentDragDropHandler::AttachComponentToObject(const char* szType, const char* szProperty, const char* szValue, ezUuid ObjectGuid)
{
  auto history = m_pDocument->GetCommandHistory();

  ezUuid CmpGuid;
  CmpGuid.CreateNewUuid();

  ezAddObjectCommand cmd;

  cmd.SetType(szType);
  cmd.m_sParentProperty = "Components";
  cmd.m_Index = -1;
  cmd.m_NewObjectGuid = CmpGuid;
  cmd.m_Parent = ObjectGuid;
  EZ_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_Object = CmpGuid;
  cmd2.m_sProperty = szProperty;
  cmd2.m_NewValue = szValue;
  EZ_VERIFY(history->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");
}

void ezComponentDragDropHandler::MoveObjectToPosition(const ezUuid& guid, const ezVec3& vPosition)
{
  auto history = m_pDocument->GetCommandHistory();

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_Object = guid;

  cmd2.m_sProperty = "LocalPosition";
  cmd2.m_NewValue = vPosition;
  history->AddCommand(cmd2);
}

void ezComponentDragDropHandler::MoveDraggedObjectsToPosition(ezVec3 vPosition, bool bAllowSnap)
{
  if (m_DraggedObjects.IsEmpty() || vPosition.IsNaN())
    return;

  if (bAllowSnap)
  {
    ezSnapProvider::SnapTranslation(vPosition);
  }

  auto history = m_pDocument->GetCommandHistory();

  history->StartTransaction("Move to Position");

  for (const auto& guid : m_DraggedObjects)
  {
    MoveObjectToPosition(guid, vPosition);
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

  m_pDocument->GetSelectionManager()->SetSelection(NewSel);
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

  m_pDocument->GetSelectionManager()->Clear();

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

  MoveDraggedObjectsToPosition(vPos, !pInfo->m_bCtrlKeyDown);
}

void ezComponentDragDropHandler::OnDragCancel()
{
  CancelTemporaryCommands();
  m_pDocument->GetCommandHistory()->CancelTransaction();

  m_DraggedObjects.Clear();
}

void ezComponentDragDropHandler::OnDrop(const ezDragDropInfo* pInfo)
{
  EndTemporaryCommands();
  m_pDocument->GetCommandHistory()->FinishTransaction();

  SelectCreatedObjects();

  m_DraggedObjects.Clear();
}

float ezComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext != "viewport" &&
      pInfo->m_sTargetContext != "scenetree")
    return 0.0f;

  const ezDocument* pDocument = ezDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);

  const ezRTTI* pRttiScene = ezRTTI::FindTypeByName("ezSceneDocument");

  if (pRttiScene == nullptr)
    return 0.0f;

  if (!pDocument->GetDynamicRTTI()->IsDerivedFrom(pRttiScene))
    return 0.0f;

  return 1.0f;
}
