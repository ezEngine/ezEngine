#include <PCH.h>
#include <EditorPluginScene/DragDropHandlers/ComponentDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

#include <QMimeData>
#include <QDataStream>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezComponentDragDropHandler, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezComponentDragDropHandler::CreateDropObject(const ezVec3& vPosition, const char* szType, const char* szProperty, const char* szValue)
{
  ezUuid ObjectGuid, CmpGuid;
  ObjectGuid.CreateNewUuid();
  CmpGuid.CreateNewUuid();

  ezAddObjectCommand cmd;
  cmd.SetType("ezGameObject");
  cmd.m_NewObjectGuid = ObjectGuid;
  cmd.m_Index = -1;
  cmd.m_sParentProperty = "Children";

  auto history = m_pDocument->GetCommandHistory();

  history->AddCommand(cmd);

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_Object = ObjectGuid;

  cmd2.SetPropertyPath("LocalPosition");
  cmd2.m_NewValue = vPosition;
  history->AddCommand(cmd2);

  cmd.SetType(szType);
  cmd.m_sParentProperty = "Components";
  cmd.m_Index = -1;
  cmd.m_NewObjectGuid = CmpGuid;
  cmd.m_Parent = ObjectGuid;
  history->AddCommand(cmd);

  cmd2.m_Object = CmpGuid;
  cmd2.SetPropertyPath(szProperty);
  cmd2.m_NewValue = szValue;
  history->AddCommand(cmd2);

  m_DraggedObjects.PushBack(ObjectGuid);
}

void ezComponentDragDropHandler::MoveObjectToPosition(const ezUuid& guid, const ezVec3& vPosition)
{
  auto history = m_pDocument->GetCommandHistory();

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_Object = guid;

  cmd2.SetPropertyPath("LocalPosition");
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

  history->StartTransaction();

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
  m_pDocument->GetCommandHistory()->FinishTransaction();
  m_pDocument->GetCommandHistory()->BeginTemporaryCommands();
}

void ezComponentDragDropHandler::EndTemporaryCommands()
{
  m_pDocument->GetCommandHistory()->FinishTemporaryCommands();
  m_pDocument->GetCommandHistory()->MergeLastTwoTransactions();
}

void ezComponentDragDropHandler::CancelTemporaryCommands()
{
  if (m_DraggedObjects.IsEmpty())
    return;

  m_pDocument->GetSelectionManager()->Clear();

  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
  m_pDocument->GetCommandHistory()->Undo();
}

void ezComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  m_pDocument = ezDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);
  EZ_ASSERT_DEV(m_pDocument != nullptr, "Invalid document GUID in drag & drop operation");

  m_pDocument->GetCommandHistory()->StartTransaction();
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
  m_DraggedObjects.Clear();
}

void ezComponentDragDropHandler::OnDrop(const ezDragDropInfo* pInfo)
{
  EndTemporaryCommands();

  SelectCreatedObjects();

  m_DraggedObjects.Clear();
}

float ezComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext != "viewport" &&
      pInfo->m_sTargetContext != "scenetree")
    return 0.0f;

  const ezDocument* pDocument = ezDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);

  if (!pDocument->GetDynamicRTTI()->IsDerivedFrom<ezSceneDocument>())
    return 0.0f;

  return 1.0f;
}
