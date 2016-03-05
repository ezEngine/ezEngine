#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/IO/MemoryStream.h>

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentRoot, 1, ezRTTINoAllocator);
EZ_BEGIN_PROPERTIES
EZ_ARRAY_MEMBER_PROPERTY("Children", m_RootObjects)->AddFlags(ezPropertyFlags::PointerOwner),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();


void ezDocumentRootObject::InsertSubObject(ezDocumentObject* pObject, const char* szProperty, const ezVariant& index)
{
  return ezDocumentObject::InsertSubObject(pObject, "Children", index);
}

void ezDocumentRootObject::RemoveSubObject(ezDocumentObject* pObject)
{
  return ezDocumentObject::RemoveSubObject(pObject);
}

ezDocumentObjectManager::ezDocumentObjectManager()
  : m_pDocument(nullptr)
{
  m_RootObject.m_pDocumentObjectManager = this;
}

ezDocumentObjectManager::~ezDocumentObjectManager()
{
  EZ_ASSERT_DEV(m_GuidToObject.IsEmpty(), "Not all objects have been destroyed!");
}

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Object Construction / Destruction
////////////////////////////////////////////////////////////////////////

ezDocumentObject* ezDocumentObjectManager::CreateObject(const ezRTTI* pRtti, ezUuid guid)
{
  EZ_ASSERT_DEV(pRtti != nullptr, "Unknown RTTI type");

  ezDocumentObject* pObject = InternalCreateObject(pRtti);
  pObject->m_pDocumentObjectManager = this;

  if (guid.IsValid())
    pObject->m_Guid = guid;
  else
    pObject->m_Guid.CreateNewUuid();

  ezDocumentObjectEvent e;
  e.m_pObject = pObject;
  e.m_EventType = ezDocumentObjectEvent::Type::AfterObjectCreated;
  m_ObjectEvents.Broadcast(e);

  return pObject;
}

void ezDocumentObjectManager::DestroyObject(ezDocumentObject* pObject)
{
  for (ezDocumentObject* pChild : pObject->m_Children)
  {
    DestroyObject(pChild);
  }

  ezDocumentObjectEvent e;
  e.m_pObject = pObject;
  e.m_EventType = ezDocumentObjectEvent::Type::BeforeObjectDestroyed;
  m_ObjectEvents.Broadcast(e);

  InternalDestroyObject(pObject);
}

void ezDocumentObjectManager::DestroyAllObjects()
{
  for (auto child : m_RootObject.m_Children)
  {
    DestroyObject(child);
  }

  m_RootObject.m_Children.Clear();
  m_GuidToObject.Clear();
}

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Structure Change
////////////////////////////////////////////////////////////////////////

void ezDocumentObjectManager::AddObject(ezDocumentObject* pObject, ezDocumentObject* pParent, const char* szParentProperty, ezVariant index)
{
  if (pParent == nullptr)
    pParent = &m_RootObject;
  if (pParent == &m_RootObject)
    szParentProperty = "Children";

  EZ_ASSERT_DEV(pObject->GetGuid().IsValid(), "Object Guid invalid! Object was not created via an ezObjectManagerBase!");
  EZ_ASSERT_DEV(CanAdd(pObject->GetTypeAccessor().GetType(), pParent, szParentProperty, index).m_Result.Succeeded(), "Trying to execute invalid add!");

  ezDocumentObjectStructureEvent e;
  e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeObjectAdded;
  e.m_pObject = pObject;
  e.m_pPreviousParent = nullptr;
  e.m_pNewParent = pParent;
  e.m_sParentProperty = szParentProperty;
  e.m_PropertyIndex = index;

  if (e.m_PropertyIndex.CanConvertTo<ezInt32>() && e.m_PropertyIndex.ConvertTo<ezInt32>() == -1)
  {
    ezIReflectedTypeAccessor& accessor = pParent->GetTypeAccessor();
    e.m_PropertyIndex = accessor.GetCount(szParentProperty);
  }
  m_StructureEvents.Broadcast(e);

  pParent->InsertSubObject(pObject, szParentProperty, e.m_PropertyIndex);
  RecursiveAddGuids(pObject);

  e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectAdded;
  m_StructureEvents.Broadcast(e);
}

void ezDocumentObjectManager::RemoveObject(ezDocumentObject* pObject)
{
  EZ_ASSERT_DEV(CanRemove(pObject).m_Result.Succeeded(), "Trying to execute invalid remove!");
  ezPropertyPath path(pObject->m_sParentProperty);
  ezIReflectedTypeAccessor& accessor = pObject->m_pParent->GetTypeAccessor();

  ezVariant index = accessor.GetPropertyChildIndex(path, pObject->GetGuid());

  ezDocumentObjectStructureEvent e;
  e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = nullptr;
  e.m_sParentProperty = pObject->m_sParentProperty;
  e.m_PropertyIndex = index;
  m_StructureEvents.Broadcast(e);

  pObject->m_pParent->RemoveSubObject(pObject);
  RecursiveRemoveGuids(pObject);

  e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectRemoved;
  m_StructureEvents.Broadcast(e);
}

void ezDocumentObjectManager::MoveObject(ezDocumentObject* pObject, ezDocumentObject* pNewParent, const char* szParentProperty, ezVariant index)
{
  EZ_ASSERT_DEV(CanMove(pObject, pNewParent, szParentProperty, index).m_Result.Succeeded(), "Trying to execute invalid move!");

  if (pNewParent == nullptr)
    pNewParent = &m_RootObject;

  ezDocumentObjectStructureEvent e;
  e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeObjectMoved;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = pNewParent;
  e.m_sParentProperty = szParentProperty;
  e.m_PropertyIndex = index;

  if (e.m_PropertyIndex.CanConvertTo<ezInt32>() && e.m_PropertyIndex.ConvertTo<ezInt32>() == -1)
  {
    ezIReflectedTypeAccessor& accessor = pNewParent->GetTypeAccessor();
    e.m_PropertyIndex = accessor.GetCount(szParentProperty);
  }

  m_StructureEvents.Broadcast(e);

  ezVariant newIndex = e.m_PropertyIndex;
  if (pNewParent == pObject->m_pParent)
  {
    ezIReflectedTypeAccessor& accessor = pObject->m_pParent->GetTypeAccessor();

    const ezRTTI* pType = accessor.GetType();
    ezPropertyPath path(pObject->m_sParentProperty);
    auto* pProp = ezToolsReflectionUtils::GetPropertyByPath(pType, path);
    if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
    {
      ezPropertyPath path(pObject->m_sParentProperty);
      ezInt32 iCurrentIndex = accessor.GetPropertyChildIndex(path, pObject->GetGuid()).ConvertTo<ezInt32>();
      ezInt32 iNewIndex = e.m_PropertyIndex.ConvertTo<ezInt32>();
      // Move after oneself?
      if (iNewIndex > iCurrentIndex)
      {
        iNewIndex -= 1;
        newIndex = iNewIndex;
      }
    }
  }

  pObject->m_pParent->RemoveSubObject(pObject);
  pNewParent->InsertSubObject(pObject, szParentProperty, newIndex);

  e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectMoved;
  m_StructureEvents.Broadcast(e);


  e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectMoved2;
  m_StructureEvents.Broadcast(e);
}

const ezDocumentObject* ezDocumentObjectManager::GetObject(const ezUuid& guid) const
{
  const ezDocumentObject* pObject = nullptr;
  if (m_GuidToObject.TryGetValue(guid, pObject))
  {
    return pObject;
  }

  return nullptr;
}

ezDocumentObject* ezDocumentObjectManager::GetObject(const ezUuid& guid)
{
  return const_cast<ezDocumentObject*>(((const ezDocumentObjectManager*)this)->GetObject(guid));
}


////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Structure Change Test
////////////////////////////////////////////////////////////////////////

ezStatus ezDocumentObjectManager::CanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const
{
  // Test whether parent exists in tree.
  if (pParent == GetRootObject())
    pParent = nullptr;

  if (pParent != nullptr)
  {
    const ezDocumentObject* pObjectInTree = GetObject(pParent->GetGuid());
    EZ_ASSERT_DEV(pObjectInTree == pParent, "Tree Corruption!!!");
    if (pObjectInTree == nullptr)
      return ezStatus("Parent is not part of the object manager!");

    const ezIReflectedTypeAccessor& accessor = pParent->GetTypeAccessor();
    const ezRTTI* pType = accessor.GetType();
    auto* pProp = ezToolsReflectionUtils::GetPropertyByPath(pType, szParentProperty);
    if (pProp == nullptr)
      return ezStatus("Property '%s' could not be found in type '%s'", szParentProperty, pType->GetTypeName());

    if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
    {
      return ezStatus("Need to use 'InsertValue' action instead.");
    }
    else if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
    {
      if (!pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
        return ezStatus("Cannot add object to the pointer property '%s' as it does not hold ownership.", szParentProperty);

      if (!pRtti->IsDerivedFrom(pProp->GetSpecificType()))
        return ezStatus("Cannot add object to the pointer property '%s' as its type '%s' is not derived from the property type '%s'!"
          , szParentProperty, pRtti->GetTypeName(), pProp->GetSpecificType()->GetTypeName());
    }
    else
    {
      if (pRtti != pProp->GetSpecificType())
        return ezStatus("Cannot add object to the property '%s' as its type '%s' does not match the property type '%s'!"
          , szParentProperty, pRtti->GetTypeName(), pProp->GetSpecificType()->GetTypeName());
    }

    if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
    {
      ezPropertyPath path(szParentProperty);
      ezInt32 iCount = accessor.GetCount(path);
      ezInt32 iNewIndex = index.ConvertTo<ezInt32>();
      if (iNewIndex >(ezInt32)iCount)
        return ezStatus("Cannot add object to its new location '%i' is out of the bounds of the parent's property range '%i'!"
          , iNewIndex, (ezInt32)iCount);
    }
    else if (pProp->GetCategory() == ezPropertyCategory::Member)
    {
      ezPropertyPath path(szParentProperty);
      ezVariant value = accessor.GetValue(path);
      if (!value.IsA<ezUuid>())
        return ezStatus("Property is not a pointer and thus can't be added to.");
 
      if (value.Get<ezUuid>().IsValid())
        return ezStatus("Can't set pointer if it already has a value, need to delete value first.");
    }
  }

  return InternalCanAdd(pRtti, pParent, szParentProperty, index);
}

ezStatus ezDocumentObjectManager::CanRemove(const ezDocumentObject* pObject) const
{
  const ezDocumentObject* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return ezStatus("Object is not part of the object manager!");

  EZ_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  return InternalCanRemove(pObject);
}

ezStatus ezDocumentObjectManager::CanMove(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProperty, const ezVariant& index) const
{
  ezStatus status = CanAdd(pObject->GetTypeAccessor().GetType(), pNewParent, szParentProperty, index);
  if (status.m_Result.Failed())
    return status;

  status = CanRemove(pObject);
  if (status.m_Result.Failed())
    return status;

  if (pNewParent == nullptr)
    pNewParent = GetRootObject();

  if (pObject == pNewParent)
    return ezStatus("Can't move object onto itself!");

  const ezDocumentObject* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return ezStatus("Object is not part of the object manager!");

  EZ_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  if (pNewParent != GetRootObject())
  {
    const ezDocumentObject* pNewParentInTree = GetObject(pNewParent->GetGuid());

    if (pNewParentInTree == nullptr)
      return ezStatus("New parent is not part of the object manager!");

    EZ_ASSERT_DEV(pNewParentInTree == pNewParent, "Tree Corruption!!!");
  }

  const ezDocumentObject* pCurParent = pNewParent->GetParent();

  while (pCurParent)
  {
    if (pCurParent == pObject)
      return ezStatus("Can't move object to one of its children!");

    pCurParent = pCurParent->GetParent();
  }

  const ezIReflectedTypeAccessor& accessor = pNewParent->GetTypeAccessor();
  const ezRTTI* pType = accessor.GetType();

  ezPropertyPath path(szParentProperty);
  auto* pProp = ezToolsReflectionUtils::GetPropertyByPath(pType, path);

  if (pProp == nullptr)
    return ezStatus("Property '%s' could not be found in type '%s'", szParentProperty, pType->GetTypeName());

  if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
  {
    ezInt32 iChildIndex = index.ConvertTo<ezInt32>();
    if (iChildIndex == -1)
    {
      iChildIndex = pNewParent->GetTypeAccessor().GetCount(szParentProperty);
    }

    if (pNewParent == pObject->GetParent())
    {
      // Test whether we are moving before or after ourselves, both of which are not allowed and would not change the tree.
      ezIReflectedTypeAccessor& oldAccessor = pObject->m_pParent->GetTypeAccessor();
      ezInt32 iCurrentIndex = oldAccessor.GetPropertyChildIndex(szParentProperty, pObject->GetGuid()).ConvertTo<ezInt32>();
      if (iChildIndex == iCurrentIndex || iChildIndex == iCurrentIndex + 1)
        return ezStatus("Can't move object onto itself!");
    }
  }

  if (pNewParent == GetRootObject())
    pNewParent = nullptr;

  return InternalCanMove(pObject, pNewParent, szParentProperty, index);
}

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Private Functions
////////////////////////////////////////////////////////////////////////

void ezDocumentObjectManager::RecursiveAddGuids(ezDocumentObject* pObject)
{
  m_GuidToObject[pObject->m_Guid] = pObject;

  for (ezUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveAddGuids(pObject->GetChildren()[c]);
}

void ezDocumentObjectManager::RecursiveRemoveGuids(ezDocumentObject* pObject)
{
  m_GuidToObject.Remove(pObject->m_Guid);

  for (ezUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveRemoveGuids(pObject->GetChildren()[c]);
}
