#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAddObjectCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezAddObjectCommand>);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Type", GetType, SetType), 
    EZ_MEMBER_PROPERTY("ParentGuid", m_Parent),
    EZ_MEMBER_PROPERTY("ParentProperty", m_sParentProperty),
    EZ_MEMBER_PROPERTY("Index", m_Index),
    EZ_MEMBER_PROPERTY("NewGuid", m_NewObjectGuid),
    EZ_MEMBER_PROPERTY("EditorProperty", m_bEditorProperty),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRemoveObjectCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezRemoveObjectCommand>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMoveObjectCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezMoveObjectCommand>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
    EZ_MEMBER_PROPERTY("NewParentGuid", m_NewParent),
    EZ_MEMBER_PROPERTY("ParentProperty", m_sParentProperty),
    EZ_MEMBER_PROPERTY("Index", m_Index),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSetObjectPropertyCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezSetObjectPropertyCommand>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
    EZ_MEMBER_PROPERTY("NewValue", m_NewValue),
    EZ_MEMBER_PROPERTY("EditorProperty", m_bEditorProperty),
    EZ_ACCESSOR_PROPERTY("PropertyPath", GetPropertyPath, SetPropertyPath),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInsertObjectPropertyCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezInsertObjectPropertyCommand>);
EZ_BEGIN_PROPERTIES
  EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
  EZ_MEMBER_PROPERTY("NewValue", m_NewValue),
  EZ_MEMBER_PROPERTY("Index", m_Index),
  EZ_MEMBER_PROPERTY("EditorProperty", m_bEditorProperty),
  EZ_ACCESSOR_PROPERTY("PropertyPath", GetPropertyPath, SetPropertyPath),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRemoveObjectPropertyCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezRemoveObjectPropertyCommand>);
EZ_BEGIN_PROPERTIES
  EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
  EZ_MEMBER_PROPERTY("Index", m_Index),
  EZ_MEMBER_PROPERTY("EditorProperty", m_bEditorProperty),
  EZ_ACCESSOR_PROPERTY("PropertyPath", GetPropertyPath, SetPropertyPath),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

////////////////////////////////////////////////////////////////////////
// ezAddObjectCommand
////////////////////////////////////////////////////////////////////////

ezAddObjectCommand::ezAddObjectCommand() :
  m_pType(nullptr), m_bEditorProperty(false), m_pObject(nullptr)
{
}

const char* ezAddObjectCommand::GetType() const
{
  if (m_pType == nullptr)
    return "";

  return m_pType->GetTypeName();
}

void ezAddObjectCommand::SetType(const char* szType)
{
  m_pType = ezRTTI::FindTypeByName(szType);
}

ezStatus ezAddObjectCommand::Do(bool bRedo)
{
  ezDocumentBase* pDocument = GetDocument();

  if (!m_NewObjectGuid.IsValid())
    m_NewObjectGuid.CreateNewUuid();

  ezDocumentObjectBase* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return ezStatus(EZ_FAILURE, "Add Object: The given parent does not exist!");
  }

  if (!pDocument->GetObjectManager()->CanAdd(m_pType, pParent, m_sParentProperty, m_Index, m_bEditorProperty))
  {
    ezStringBuilder sErrorMessage;
    sErrorMessage.Format("Add Object: The type '%s' cannot be added to the given parent!", m_pType->GetTypeName());
    return ezStatus(EZ_FAILURE, sErrorMessage);
  }

  if (!bRedo)
  {
    if (pParent)
    {
      ezIReflectedTypeAccessor& accessor = m_bEditorProperty ? pParent->GetEditorTypeAccessor() : pParent->GetTypeAccessor();
      auto* pProp = accessor.GetType()->FindPropertyByName(m_sParentProperty);
      if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        m_pObject = pDocument->GetObjectManager()->CreateObject(m_pType, m_NewObjectGuid);
      }
      else
      {
        if (pProp->GetCategory() == ezPropertyCategory::Array)
          m_pObject = pDocument->GetObjectManager()->CreateSubObject(m_pType, ezDocumentObjectType::ArrayElement, m_NewObjectGuid);
        else if (pProp->GetCategory() == ezPropertyCategory::Set)
          m_pObject = pDocument->GetObjectManager()->CreateSubObject(m_pType, ezDocumentObjectType::SetElement, m_NewObjectGuid);
        else
        {
          EZ_ASSERT_NOT_IMPLEMENTED;
        }
      }
    }
    else
    {
      m_pObject = pDocument->GetObjectManager()->CreateObject(m_pType, m_NewObjectGuid);
    }
  }

  pDocument->GetObjectManager()->AddObject(m_pObject, pParent, m_sParentProperty, m_Index, m_bEditorProperty);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezAddObjectCommand::Undo(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  ezDocumentBase* pDocument = GetDocument();
  if (!pDocument->GetObjectManager()->CanRemove(m_pObject))
    return ezStatus(EZ_FAILURE, "Add Object: Removal of the object is forbidden!");

  pDocument->GetObjectManager()->RemoveObject(m_pObject);
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


////////////////////////////////////////////////////////////////////////
// ezRemoveObjectCommand
////////////////////////////////////////////////////////////////////////

ezRemoveObjectCommand::ezRemoveObjectCommand() :
  m_pParent(nullptr),
  m_pObject(nullptr),
  m_bEditorProperty(false)
{
}

ezStatus ezRemoveObjectCommand::Do(bool bRedo)
{
  ezDocumentBase* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
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
    m_sParentProperty = m_pObject->GetParentProperty();
    m_bEditorProperty = m_pObject->IsEditorProperty();
    const ezIReflectedTypeAccessor& accessor = m_bEditorProperty ? m_pObject->GetParent()->GetEditorTypeAccessor() : m_pObject->GetParent()->GetTypeAccessor();
    m_Index = accessor.GetPropertyChildIndex(m_pObject->GetParentProperty(), m_pObject->GetGuid());
  }

  pDocument->GetObjectManager()->RemoveObject(m_pObject);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezRemoveObjectCommand::Undo(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  ezDocumentBase* pDocument = GetDocument();
  if (!pDocument->GetObjectManager()->CanAdd(m_pObject->GetTypeAccessor().GetType(), m_pParent, m_sParentProperty, m_Index, m_bEditorProperty))
    return ezStatus(EZ_FAILURE, "Remove Object: Adding the object is forbidden!");

  pDocument->GetObjectManager()->AddObject(m_pObject, m_pParent, m_sParentProperty, m_Index, m_bEditorProperty);
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


////////////////////////////////////////////////////////////////////////
// ezMoveObjectCommand
////////////////////////////////////////////////////////////////////////

ezMoveObjectCommand::ezMoveObjectCommand()
{
  m_pObject = nullptr;
  m_pOldParent = nullptr;
  m_pNewParent = nullptr;
}

ezStatus ezMoveObjectCommand::Do(bool bRedo)
{
  ezDocumentBase* pDocument = GetDocument();

  if (!bRedo)
  {
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return ezStatus(EZ_FAILURE, "Move Object: The given object does not exist!");
    }

    if (m_NewParent.IsValid())
    {
      m_pNewParent = pDocument->GetObjectManager()->GetObject(m_NewParent);
      if (m_pNewParent == nullptr)
        return ezStatus(EZ_FAILURE, "Move Object: The new parent does not exist!");
    }

    m_pOldParent = const_cast<ezDocumentObjectBase*>(m_pObject->GetParent());
    m_sOldParentProperty = m_pObject->GetParentProperty();
    const ezIReflectedTypeAccessor& accessor = m_pObject->IsEditorProperty() ? m_pOldParent->GetEditorTypeAccessor() : m_pOldParent->GetTypeAccessor();
    m_OldIndex = accessor.GetPropertyChildIndex(m_pObject->GetParentProperty(), m_pObject->GetGuid());

    if (!pDocument->GetObjectManager()->CanMove(m_pObject, m_pNewParent, m_sParentProperty, m_Index))
    {
      return ezStatus(EZ_FAILURE, "Move Object: Cannot move object to the new location.");
    }
  }

  pDocument->GetObjectManager()->MoveObject(m_pObject, m_pNewParent, m_sParentProperty, m_Index);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMoveObjectCommand::Undo(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  
  ezDocumentBase* pDocument = GetDocument();

  ezVariant FinalOldPosition = m_OldIndex;

  if (m_Index.CanConvertTo<ezInt32>() && m_pOldParent == m_pNewParent)
  {
    // If we are moving an object downwards, we must move by more than 1 (+1 would be behind the same object, which is still the same position)
    // so an object must always be moved by at least +2
    // moving UP can be done by -1, so when we undo that, we must ensure to move +2

    ezInt32 iNew = m_Index.ConvertTo<ezInt32>();
    ezInt32 iOld = m_OldIndex.ConvertTo<ezInt32>();

    if (iNew < iOld)
    {
      FinalOldPosition = iOld + 1;
    }
  }

  if (!pDocument->GetObjectManager()->CanMove(m_pObject, m_pOldParent, m_sOldParentProperty, FinalOldPosition))
  {
    return ezStatus(EZ_FAILURE, "Move Object: Cannot move object to the old location.");
  }

  pDocument->GetObjectManager()->MoveObject(m_pObject, m_pOldParent, m_sOldParentProperty, FinalOldPosition);

  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezSetObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

ezSetObjectPropertyCommand::ezSetObjectPropertyCommand()
{
  m_bEditorProperty = false;
  m_pObject = nullptr;
}

ezStatus ezSetObjectPropertyCommand::Do(bool bRedo)
{
  ezDocumentBase* pDocument = GetDocument();
  ezPropertyPath path(m_sPropertyPath);

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return ezStatus(EZ_FAILURE, "Set Property: The given object does not exist!");
    }
    else
      return ezStatus(EZ_FAILURE, "Set Property: The given object does not exist!");

    ezIReflectedTypeAccessor& accessor0 = m_bEditorProperty ? m_pObject->GetEditorTypeAccessor() : m_pObject->GetTypeAccessor();
    m_OldValue = accessor0.GetValue(path);
    if (!m_OldValue.IsValid())
      return ezStatus("Set Property: The property '%s' does not exist", m_sPropertyPath.GetData());
  }

  ezIReflectedTypeAccessor& accessor = m_bEditorProperty ? m_pObject->GetEditorTypeAccessor() : m_pObject->GetTypeAccessor();
  if (!accessor.SetValue(path, m_NewValue))
  {
    return ezStatus("Set Property: The property '%s' does not exist", m_sPropertyPath.GetData());
  }

  ezDocumentObjectPropertyEvent e;
  e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertySet;
  e.m_bEditorProperty = m_bEditorProperty;
  e.m_pObject = m_pObject;
  e.m_OldValue = m_OldValue;
  e.m_NewValue = m_NewValue;
  e.m_sPropertyPath = m_sPropertyPath;

  pDocument->GetObjectManager()->m_PropertyEvents.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSetObjectPropertyCommand::Undo(bool bFireEvents)
{
  ezIReflectedTypeAccessor& accessor = m_bEditorProperty ? m_pObject->GetEditorTypeAccessor() : m_pObject->GetTypeAccessor();
  ezPropertyPath path(m_sPropertyPath);

  if (!accessor.SetValue(path, m_OldValue))
  {
    return ezStatus("Set Property: The property '%s' does not exist", m_sPropertyPath.GetData());
  }

  if (bFireEvents)
  {
    ezDocumentObjectPropertyEvent e;
    e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertySet;
    e.m_bEditorProperty = m_bEditorProperty;
    e.m_pObject = m_pObject;
    e.m_OldValue = m_NewValue;
    e.m_NewValue = m_OldValue;
    e.m_sPropertyPath = m_sPropertyPath;

    GetDocument()->GetObjectManager()->m_PropertyEvents.Broadcast(e);
  }

  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezInsertObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

ezInsertObjectPropertyCommand::ezInsertObjectPropertyCommand()
{
  m_bEditorProperty = false;
  m_pObject = nullptr;
}

ezStatus ezInsertObjectPropertyCommand::Do(bool bRedo)
{
  ezDocumentBase* pDocument = GetDocument();
  ezPropertyPath path(m_sPropertyPath);

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return ezStatus(EZ_FAILURE, "Insert Property: The given object does not exist!");
    }
    else
      return ezStatus(EZ_FAILURE, "Insert Property: The given object does not exist!");
  }

  ezIReflectedTypeAccessor& accessor = m_bEditorProperty ? m_pObject->GetEditorTypeAccessor() : m_pObject->GetTypeAccessor();
  if (!accessor.InsertValue(path, m_Index, m_NewValue))
  {
    return ezStatus("Insert Property: The property '%s' does not exist", m_sPropertyPath.GetData());
  }

  ezDocumentObjectPropertyEvent e;
  e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyInserted;
  e.m_bEditorProperty = m_bEditorProperty;
  e.m_pObject = m_pObject;
  e.m_NewValue = m_NewValue;
  e.m_Index = m_Index;
  e.m_sPropertyPath = m_sPropertyPath;

  pDocument->GetObjectManager()->m_PropertyEvents.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezInsertObjectPropertyCommand::Undo(bool bFireEvents)
{
  ezIReflectedTypeAccessor& accessor = m_bEditorProperty ? m_pObject->GetEditorTypeAccessor() : m_pObject->GetTypeAccessor();
  ezPropertyPath path(m_sPropertyPath);

  if (!accessor.RemoveValue(path, m_Index))
  {
    return ezStatus("Insert Property: The property '%s' does not exist", m_sPropertyPath.GetData());
  }

  if (bFireEvents)
  {
    ezDocumentObjectPropertyEvent e;
    e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyRemoved;
    e.m_bEditorProperty = m_bEditorProperty;
    e.m_pObject = m_pObject;
    e.m_OldValue = m_NewValue;
    e.m_Index = m_Index;
    e.m_sPropertyPath = m_sPropertyPath;

    GetDocument()->GetObjectManager()->m_PropertyEvents.Broadcast(e);
  }

  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezRemoveObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

ezRemoveObjectPropertyCommand::ezRemoveObjectPropertyCommand()
{
  m_bEditorProperty = false;
  m_pObject = nullptr;
}

ezStatus ezRemoveObjectPropertyCommand::Do(bool bRedo)
{
  ezDocumentBase* pDocument = GetDocument();
  ezPropertyPath path(m_sPropertyPath);

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return ezStatus(EZ_FAILURE, "Remove Property: The given object does not exist!");
    }
    else
      return ezStatus(EZ_FAILURE, "Remove Property: The given object does not exist!");
  }

  ezIReflectedTypeAccessor& accessor = m_bEditorProperty ? m_pObject->GetEditorTypeAccessor() : m_pObject->GetTypeAccessor();
  m_OldValue = accessor.GetValue(path, m_Index);
  if (!m_OldValue.IsValid())
    return ezStatus("Remove Property: The index '%s' in property '%s' does not exist", m_Index.ConvertTo<ezString>().GetData(), m_sPropertyPath.GetData());

  if (!accessor.RemoveValue(path, m_Index))
  {
    return ezStatus("Remove Property: The index '%s' in property '%s' does not exist!", m_Index.ConvertTo<ezString>().GetData(), m_sPropertyPath.GetData());
  }

  ezDocumentObjectPropertyEvent e;
  e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyInserted;
  e.m_bEditorProperty = m_bEditorProperty;
  e.m_pObject = m_pObject;
  e.m_OldValue = m_OldValue;
  e.m_Index = m_Index;
  e.m_sPropertyPath = m_sPropertyPath;

  pDocument->GetObjectManager()->m_PropertyEvents.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezRemoveObjectPropertyCommand::Undo(bool bFireEvents)
{
  ezIReflectedTypeAccessor& accessor = m_bEditorProperty ? m_pObject->GetEditorTypeAccessor() : m_pObject->GetTypeAccessor();
  ezPropertyPath path(m_sPropertyPath);

  if (!accessor.InsertValue(path, m_Index, m_OldValue))
  {
    return ezStatus("Remove Property: Undo failed! The index '%s' in property '%s' does not exist", m_Index.ConvertTo<ezString>().GetData(), m_sPropertyPath.GetData());
  }

  if (bFireEvents)
  {
    ezDocumentObjectPropertyEvent e;
    e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyRemoved;
    e.m_bEditorProperty = m_bEditorProperty;
    e.m_pObject = m_pObject;
    e.m_NewValue = m_OldValue;
    e.m_Index = m_Index;
    e.m_sPropertyPath = m_sPropertyPath;

    GetDocument()->GetObjectManager()->m_PropertyEvents.Broadcast(e);
  }

  return ezStatus(EZ_SUCCESS);
}

