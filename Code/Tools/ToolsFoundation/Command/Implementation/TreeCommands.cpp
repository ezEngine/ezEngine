#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAddObjectCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezAddObjectCommand>);
EZ_BEGIN_PROPERTIES
EZ_ACCESSOR_PROPERTY("Type", GetType, SetType),
EZ_MEMBER_PROPERTY("ParentGuid", m_Parent),
EZ_MEMBER_PROPERTY("ParentProperty", m_sParentProperty),
EZ_MEMBER_PROPERTY("Index", m_Index),
EZ_MEMBER_PROPERTY("NewGuid", m_NewObjectGuid),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPasteObjectsCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezPasteObjectsCommand>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("ParentGuid", m_Parent),
EZ_MEMBER_PROPERTY("JsonGraph", m_sJsonGraph),
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
EZ_MEMBER_PROPERTY("Index", m_Index),
EZ_ACCESSOR_PROPERTY("PropertyPath", GetPropertyPath, SetPropertyPath),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInsertObjectPropertyCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezInsertObjectPropertyCommand>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
EZ_MEMBER_PROPERTY("NewValue", m_NewValue),
EZ_MEMBER_PROPERTY("Index", m_Index),
EZ_ACCESSOR_PROPERTY("PropertyPath", GetPropertyPath, SetPropertyPath),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRemoveObjectPropertyCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezRemoveObjectPropertyCommand>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
EZ_MEMBER_PROPERTY("Index", m_Index),
EZ_ACCESSOR_PROPERTY("PropertyPath", GetPropertyPath, SetPropertyPath),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMoveObjectPropertyCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezMoveObjectPropertyCommand>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
EZ_MEMBER_PROPERTY("OldIndex", m_OldIndex),
EZ_MEMBER_PROPERTY("NewIndex", m_NewIndex),
EZ_ACCESSOR_PROPERTY("PropertyPath", GetPropertyPath, SetPropertyPath),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

////////////////////////////////////////////////////////////////////////
// ezAddObjectCommand
////////////////////////////////////////////////////////////////////////

ezAddObjectCommand::ezAddObjectCommand() :
  m_pType(nullptr), m_pObject(nullptr)
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

  if (!pDocument->GetObjectManager()->CanAdd(m_pType, pParent, m_sParentProperty, m_Index))
  {
    ezStringBuilder sErrorMessage;
    sErrorMessage.Format("Add Object: The type '%s' cannot be added to the given parent!", m_pType->GetTypeName());
    return ezStatus(EZ_FAILURE, sErrorMessage);
  }

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->CreateObject(m_pType, m_NewObjectGuid);
  }

  pDocument->GetObjectManager()->AddObject(m_pObject, pParent, m_sParentProperty, m_Index);
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
// ezPasteObjectsCommand
////////////////////////////////////////////////////////////////////////

ezPasteObjectsCommand::ezPasteObjectsCommand()
{
}

ezStatus ezPasteObjectsCommand::Do(bool bRedo)
{
  ezDocumentBase* pDocument = GetDocument();

  ezDocumentObjectBase* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return ezStatus(EZ_FAILURE, "Paste Objects: The given parent does not exist!");
  }

  if (!bRedo)
  {
    ezAbstractObjectGraph graph;

    {
      // Deserialize 
      ezMemoryStreamStorage streamStorage;
      ezMemoryStreamWriter memoryWriter(&streamStorage);
      memoryWriter.WriteBytes(m_sJsonGraph.GetData(), m_sJsonGraph.GetElementCount());

      ezMemoryStreamReader memoryReader(&streamStorage);
      ezAbstractGraphJsonSerializer::Read(memoryReader, &graph);
    }

    // Remap
    ezUuid seed;
    seed.CreateNewUuid();
    graph.ReMapNodeGuids(seed);

    ezDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateOnly);

    ezHybridArray<ezDocumentBase::PasteInfo, 16> ToBePasted;

    auto& nodes = graph.GetAllNodes();
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      auto* pNode = it.Value();
      if (ezStringUtils::IsEqual(pNode->GetNodeName(), "root"))
      {
        auto* pNewObject = reader.CreateObjectFromNode(pNode, nullptr, nullptr, ezVariant());
        reader.ApplyPropertiesToObject(pNode, pNewObject);

        auto& ref = ToBePasted.ExpandAndGetRef();
        ref.m_pObject = pNewObject;
        ref.m_pParent = pParent;
      }
    }

    if (pDocument->Paste(ToBePasted))
    {
      for (const auto& item : ToBePasted)
      {
        auto& po = m_PastedObjects.ExpandAndGetRef();
        po.m_pObject = item.m_pObject;
        po.m_pParent = item.m_pParent;
        po.m_Index = item.m_pObject->GetPropertyIndex();
        po.m_sParentProperty = item.m_pObject->GetParentProperty();
      }
    }
    else
    {
      for (const auto& item : ToBePasted)
      {
        pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
      }
    }

    if (m_PastedObjects.IsEmpty())
      return ezStatus(EZ_FAILURE, "Paste Objects: nothing was pasted!");
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_PastedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezPasteObjectsCommand::Undo(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  ezDocumentBase* pDocument = GetDocument();

  for (auto& po : m_PastedObjects)
  {
    if (!pDocument->GetObjectManager()->CanRemove(po.m_pObject))
      return ezStatus(EZ_FAILURE, "Add Object: Removal of the object is forbidden!");

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return ezStatus(EZ_SUCCESS);
}

void ezPasteObjectsCommand::Cleanup(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_PastedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_PastedObjects.Clear();
  }

}


////////////////////////////////////////////////////////////////////////
// ezRemoveObjectCommand
////////////////////////////////////////////////////////////////////////

ezRemoveObjectCommand::ezRemoveObjectCommand() :
  m_pParent(nullptr),
  m_pObject(nullptr)
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

      /// \todo BLA
      sErrorMessage.Format("Remove Object: The object '%s' cannot be remove!", m_pObject->GetTypeAccessor().GetValue("Name").GetData());
      return ezStatus(EZ_FAILURE, sErrorMessage);
    }

    m_pParent = const_cast<ezDocumentObjectBase*>(m_pObject->GetParent());
    m_sParentProperty = m_pObject->GetParentProperty();
    const ezIReflectedTypeAccessor& accessor = m_pObject->GetParent()->GetTypeAccessor();
    m_Index = accessor.GetPropertyChildIndex(m_pObject->GetParentProperty(), m_pObject->GetGuid());
  }

  pDocument->GetObjectManager()->RemoveObject(m_pObject);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezRemoveObjectCommand::Undo(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  ezDocumentBase* pDocument = GetDocument();
  if (!pDocument->GetObjectManager()->CanAdd(m_pObject->GetTypeAccessor().GetType(), m_pParent, m_sParentProperty, m_Index))
    return ezStatus(EZ_FAILURE, "Remove Object: Adding the object is forbidden!");

  pDocument->GetObjectManager()->AddObject(m_pObject, m_pParent, m_sParentProperty, m_Index);
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
    const ezIReflectedTypeAccessor& accessor = m_pOldParent->GetTypeAccessor();
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

    ezIReflectedTypeAccessor& accessor0 = m_pObject->GetTypeAccessor();
    m_OldValue = accessor0.GetValue(path, m_Index);
    if (!m_OldValue.IsValid())
      return ezStatus("Set Property: The property '%s' does not exist", m_sPropertyPath.GetData());
  }

  ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
  if (!accessor.SetValue(path, m_NewValue, m_Index))
  {
    return ezStatus("Set Property: The property '%s' does not exist", m_sPropertyPath.GetData());
  }

  ezDocumentObjectPropertyEvent e;
  e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertySet;
  e.m_pObject = m_pObject;
  e.m_OldValue = m_OldValue;
  e.m_NewValue = m_NewValue;
  e.m_sPropertyPath = m_sPropertyPath;
  e.m_NewIndex = m_Index;

  pDocument->GetObjectManager()->m_PropertyEvents.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSetObjectPropertyCommand::Undo(bool bFireEvents)
{
  ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
  ezPropertyPath path(m_sPropertyPath);

  if (!accessor.SetValue(path, m_OldValue, m_Index))
  {
    return ezStatus("Set Property: The property '%s' does not exist", m_sPropertyPath.GetData());
  }

  if (bFireEvents)
  {
    ezDocumentObjectPropertyEvent e;
    e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertySet;
    e.m_pObject = m_pObject;
    e.m_OldValue = m_NewValue;
    e.m_NewValue = m_OldValue;
    e.m_sPropertyPath = m_sPropertyPath;
    e.m_NewIndex = m_Index;

    GetDocument()->GetObjectManager()->m_PropertyEvents.Broadcast(e);
  }

  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezInsertObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

ezInsertObjectPropertyCommand::ezInsertObjectPropertyCommand()
{
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

    if (m_Index.CanConvertTo<ezInt32>() && m_Index.ConvertTo<ezInt32>() == -1)
    {
      ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
      m_Index = accessor.GetCount(m_sPropertyPath.GetData());
    }
  }

  ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
  if (!accessor.InsertValue(path, m_Index, m_NewValue))
  {
    return ezStatus("Insert Property: The property '%s' does not exist", m_sPropertyPath.GetData());
  }

  ezDocumentObjectPropertyEvent e;
  e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyInserted;
  e.m_pObject = m_pObject;
  e.m_NewValue = m_NewValue;
  e.m_NewIndex = m_Index;
  e.m_sPropertyPath = m_sPropertyPath;

  pDocument->GetObjectManager()->m_PropertyEvents.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezInsertObjectPropertyCommand::Undo(bool bFireEvents)
{
  ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
  ezPropertyPath path(m_sPropertyPath);

  if (!accessor.RemoveValue(path, m_Index))
  {
    return ezStatus("Insert Property: The property '%s' does not exist", m_sPropertyPath.GetData());
  }

  if (bFireEvents)
  {
    ezDocumentObjectPropertyEvent e;
    e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyRemoved;
    e.m_pObject = m_pObject;
    e.m_OldValue = m_NewValue;
    e.m_OldIndex = m_Index;
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

  ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
  m_OldValue = accessor.GetValue(path, m_Index);
  if (!m_OldValue.IsValid())
    return ezStatus("Remove Property: The index '%s' in property '%s' does not exist", m_Index.ConvertTo<ezString>().GetData(), m_sPropertyPath.GetData());

  if (!accessor.RemoveValue(path, m_Index))
  {
    return ezStatus("Remove Property: The index '%s' in property '%s' does not exist!", m_Index.ConvertTo<ezString>().GetData(), m_sPropertyPath.GetData());
  }

  ezDocumentObjectPropertyEvent e;
  e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyRemoved;
  e.m_pObject = m_pObject;
  e.m_OldValue = m_OldValue;
  e.m_OldIndex = m_Index;
  e.m_sPropertyPath = m_sPropertyPath;

  pDocument->GetObjectManager()->m_PropertyEvents.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezRemoveObjectPropertyCommand::Undo(bool bFireEvents)
{
  ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
  ezPropertyPath path(m_sPropertyPath);

  if (!accessor.InsertValue(path, m_Index, m_OldValue))
  {
    return ezStatus("Remove Property: Undo failed! The index '%s' in property '%s' does not exist", m_Index.ConvertTo<ezString>().GetData(), m_sPropertyPath.GetData());
  }

  if (bFireEvents)
  {
    ezDocumentObjectPropertyEvent e;
    e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyInserted;
    e.m_pObject = m_pObject;
    e.m_NewValue = m_OldValue;
    e.m_NewIndex = m_Index;
    e.m_sPropertyPath = m_sPropertyPath;

    GetDocument()->GetObjectManager()->m_PropertyEvents.Broadcast(e);
  }

  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezMoveObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

ezMoveObjectPropertyCommand::ezMoveObjectPropertyCommand()
{
  m_pObject = nullptr;
}

ezStatus ezMoveObjectPropertyCommand::Do(bool bRedo)
{
  ezDocumentBase* pDocument = GetDocument();
  if (!m_OldIndex.CanConvertTo<ezInt32>() || !m_OldIndex.CanConvertTo<ezInt32>())
    return ezStatus(EZ_FAILURE, "Move Property: Invalid indices provided.");

  ezPropertyPath path(m_sPropertyPath.GetData());
  if (!bRedo)
  {
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return ezStatus(EZ_FAILURE, "Move Property: The given object does not exist.");
    }

    const ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    ezInt32 iCount = accessor.GetCount(path);
    if (iCount < 0)
      return ezStatus("Move Property: Invalid property.");
    if (m_OldIndex.ConvertTo<ezInt32>() < 0 || m_OldIndex.ConvertTo<ezInt32>() >= iCount)
      return ezStatus("Move Property: Invalid old index '%i'.", m_OldIndex.ConvertTo<ezInt32>());
    if (m_NewIndex.ConvertTo<ezInt32>() < 0 || m_NewIndex.ConvertTo<ezInt32>() > iCount)
      return ezStatus("Move Property: Invalid new index '%i'.", m_NewIndex.ConvertTo<ezInt32>());
  }

  ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
  if (!accessor.MoveValue(path, m_OldIndex, m_NewIndex))
    return ezStatus("Move Property: Move value failed.");

  {
    ezDocumentObjectPropertyEvent e;
    e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyMoved;
    e.m_pObject = m_pObject;
    e.m_OldIndex = m_OldIndex;
    e.m_NewIndex = m_NewIndex;
    e.m_sPropertyPath = m_sPropertyPath;

    GetDocument()->GetObjectManager()->m_PropertyEvents.Broadcast(e);
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMoveObjectPropertyCommand::Undo(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  ezDocumentBase* pDocument = GetDocument();

  ezVariant FinalOldPosition = m_OldIndex;
  ezVariant FinalNewPosition = m_NewIndex;

  if (m_OldIndex.CanConvertTo<ezInt32>())
  {
    // If we are moving an object downwards, we must move by more than 1 (+1 would be behind the same object, which is still the same position)
    // so an object must always be moved by at least +2
    // moving UP can be done by -1, so when we undo that, we must ensure to move +2

    ezInt32 iNew = m_NewIndex.ConvertTo<ezInt32>();
    ezInt32 iOld = m_OldIndex.ConvertTo<ezInt32>();

    if (iNew < iOld)
    {
      FinalOldPosition = iOld + 1;
    }

    // The new position is relative to the original array, so we need to substract one to account for
    // the removal of the same element at the lower index.
    if (iNew > iOld)
    {
      FinalNewPosition = iNew - 1;
    }
  }

  ezPropertyPath path(m_sPropertyPath.GetData());
  ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
  if (!accessor.MoveValue(path, FinalNewPosition, FinalOldPosition))
    return ezStatus("Move Property: Move value failed.");

  {
    ezDocumentObjectPropertyEvent e;
    e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyMoved;
    e.m_pObject = m_pObject;
    e.m_OldIndex = FinalNewPosition;
    e.m_NewIndex = FinalOldPosition;
    e.m_sPropertyPath = m_sPropertyPath;

    GetDocument()->GetObjectManager()->m_PropertyEvents.Broadcast(e);
  }

  return ezStatus(EZ_SUCCESS);
}
