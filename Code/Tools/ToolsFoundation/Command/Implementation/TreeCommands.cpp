#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAddObjectCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezAddObjectCommand>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("ParentGuid", m_Parent),
    EZ_MEMBER_PROPERTY("ChildIndex", m_iChildIndex),
    EZ_ACCESSOR_PROPERTY("Type", GetType, SetType),
    EZ_MEMBER_PROPERTY("Variant", m_Variant),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRemoveObjectCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezRemoveObjectCommand>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

const char* ezAddObjectCommand::GetType() const
{
  if (m_hType.IsInvalidated())
    return "";

  return m_hType.GetType()->GetTypeName().GetData();
}

void ezAddObjectCommand::SetType(const char* szType)
{
  m_hType = ezReflectedTypeManager::GetTypeHandleByName(szType);
}

ezAddObjectCommand::ezAddObjectCommand() :
  m_iChildIndex(-1),
  m_pObject(nullptr)
{
  m_Variant = ezColor::GetCornflowerBlue(); // The original!
}

ezStatus ezAddObjectCommand::Do(bool bRedo)
{
  ezDocumentBase* pDocument = GetDocument();

  ezDocumentObjectBase* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectTree()->GetObject(m_Parent);
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

void ezAddObjectCommand::Cleanup(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    GetDocument()->GetObjectManager()->DestroyObject(m_pObject);
    m_pObject = nullptr;
  }

}




ezRemoveObjectCommand::ezRemoveObjectCommand() :
  m_iChildIndex(-1),
  m_pObject(nullptr),
  m_pParent(nullptr)
{
}

ezStatus ezRemoveObjectCommand::Do(bool bRedo)
{
  ezDocumentBase* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectTree()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return ezStatus(EZ_FAILURE, "Remove Object: The given object does not exist!");
    }
    else
      return ezStatus(EZ_FAILURE, "Remove Object: The given object does not exist!");

    if (!pDocument->GetObjectManager()->CanRemove(m_pObject))
    {
      ezStringBuilder sErrorMessage;
      sErrorMessage.Format("Remove Object: The object '%s' cannot be remove!", m_pObject->GetEditorTypeAccessor().GetValue("Name").GetData());
      return ezStatus(EZ_FAILURE, sErrorMessage);
    }

    m_pParent = const_cast<ezDocumentObjectBase*>(m_pObject->GetParent());
    m_iChildIndex = m_pObject->GetParent()->GetChildIndex(m_pObject);
  }

  pDocument->GetObjectTree()->RemoveObject(m_pObject);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezRemoveObjectCommand::Undo()
{
  ezDocumentBase* pDocument = GetDocument();
  if (!pDocument->GetObjectManager()->CanAdd(m_pObject->GetTypeAccessor().GetReflectedTypeHandle(), m_pParent))
    return ezStatus(EZ_FAILURE, "Remove Object: Adding the object is forbidden!");

  pDocument->GetObjectTree()->AddObject(m_pObject, m_pParent, m_iChildIndex);
  return ezStatus(EZ_SUCCESS);
}

void ezRemoveObjectCommand::Cleanup(CommandState state)
{
  if (state == CommandState::WasDone)
  {
    GetDocument()->GetObjectManager()->DestroyObject(m_pObject);
    m_pObject = nullptr;
  }

}

