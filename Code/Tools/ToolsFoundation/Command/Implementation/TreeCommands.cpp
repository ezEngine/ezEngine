#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Command/TreeCommands.h>


ezAddObjectCommand::ezAddObjectCommand(ezReflectedTypeHandle hType, const ezUuid& parent, ezInt32 iChildIndex) :
  m_hType(hType),
  m_parent(parent),
  m_iChildIndex(iChildIndex),
  m_pObject(nullptr)
{
}

ezStatus ezAddObjectCommand::Do(bool bRedo)
{
  ezDocumentBase* pDocument = GetDocument();

  ezDocumentObjectBase* pParent = nullptr;
  if (m_parent.IsValid())
  {
    pParent = pDocument->GetObjectTree()->GetObject(m_parent);
    if (pParent == nullptr)
      return ezStatus(EZ_FAILURE, "Add Object: The given parent does not exist!");
  }

  if (!pDocument->GetObjectManager()->CanAdd(m_hType, pParent))
  {
    ezStringBuilder sErrorMessage;
    sErrorMessage.Format("Add Object: The type '%s' cannot be added to the given parent!", m_hType.GetType()->GetTypeName().GetData());
    return ezStatus(EZ_FAILURE, sErrorMessage);
  }

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->CreateObject(m_hType);
  }

  pDocument->GetObjectTree()->AddObject(m_pObject, pParent, m_iChildIndex);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezAddObjectCommand::Undo()
{
  ezDocumentBase* pDocument = GetDocument();
  if (!pDocument->GetObjectManager()->CanRemove(m_pObject))
    return ezStatus(EZ_FAILURE, "Add Object: Removal of the object is forbidden!");

  pDocument->GetObjectTree()->RemoveObject(m_pObject);
  return ezStatus(EZ_SUCCESS);
}
