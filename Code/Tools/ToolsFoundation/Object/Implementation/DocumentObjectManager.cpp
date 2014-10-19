#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectTree.h>

ezDocumentObjectBase* ezDocumentObjectManagerBase::CreateObject(ezReflectedTypeHandle hType)
{
  ezDocumentObjectBase* pObject = InternalCreateObject(hType);

  if (pObject)
  {
    pObject->m_Guid.CreateNewUuid();
  }

  return pObject;
}

bool ezDocumentObjectManagerBase::CanAdd(ezReflectedTypeHandle hType, const ezDocumentObjectBase* pParent) const
{
  const ezDocumentObjectBase* pObjectInTree = m_pObjectTree->GetObject(pParent->GetGuid());

  if (pObjectInTree == nullptr)
    return false;

  EZ_ASSERT(pObjectInTree == pParent, "Tree Corruption!!!");

  return InternalCanAdd(hType, pParent);
}

bool ezDocumentObjectManagerBase::CanRemove(const ezDocumentObjectBase* pObject) const
{
  const ezDocumentObjectBase* pObjectInTree = m_pObjectTree->GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return false;

  EZ_ASSERT(pObjectInTree == pObject, "Tree Corruption!!!");

  return InternalCanRemove(pObject);
}

bool ezDocumentObjectManagerBase::CanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent) const
{
  if (pNewParent == nullptr)
    pNewParent = m_pObjectTree->GetRootObject();

  if (pObject == pNewParent)
    return false;
  if (pObject->GetParent() == pNewParent)
    return false;

  const ezDocumentObjectBase* pObjectInTree = m_pObjectTree->GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return false;

  EZ_ASSERT(pObjectInTree == pObject, "Tree Corruption!!!");

  const ezDocumentObjectBase* pNewParentInTree = m_pObjectTree->GetObject(pNewParent->GetGuid());

  if (pNewParent == nullptr)
    return false;

  EZ_ASSERT(pNewParentInTree == pNewParent, "Tree Corruption!!!");

  const ezDocumentObjectBase* pCurParent = pNewParent->GetParent();

  while (pCurParent)
  {
    if (pCurParent == pObject)
      return false;

    pCurParent = pCurParent->GetParent();
  }

  return InternalCanMove(pObject, pNewParent);
}


