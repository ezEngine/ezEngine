#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/IO/MemoryStream.h>

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Object Construction / Destruction
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
  EZ_ASSERT_DEV(CanAdd(pObject->GetTypeAccessor().GetType(), pParent, szParentProperty, index), "Trying to execute invalid add!");

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
  EZ_ASSERT_DEV(CanRemove(pObject), "Trying to execute invalid remove!");
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
  EZ_ASSERT_DEV(CanMove(pObject, pNewParent, szParentProperty, index), "Trying to execute invalid move!");

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

bool ezDocumentObjectManager::CanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const
{
  // Test whether parent exists in tree.
  if (pParent == GetRootObject())
    pParent = nullptr;

  if (pParent != nullptr)
  {
    const ezDocumentObject* pObjectInTree = GetObject(pParent->GetGuid());
    EZ_ASSERT_DEV(pObjectInTree == pParent, "Tree Corruption!!!");
    if (pObjectInTree == nullptr)
      return false;

    const ezIReflectedTypeAccessor& accessor = pParent->GetTypeAccessor();
    const ezRTTI* pType = accessor.GetType();
    auto* pProp = ezToolsReflectionUtils::GetPropertyByPath(pType, szParentProperty);
    if (pProp == nullptr)
      return false;

    if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
    {
      // Need to use InsertValue action instead.
      return false;
    }
    else if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
    {
      if (!pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
        return false;

      if (!pRtti->IsDerivedFrom(pProp->GetSpecificType()))
        return false;
    }
    else
    {
      if (pRtti != pProp->GetSpecificType())
        return false;
    }

    if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
    {
      ezPropertyPath path(szParentProperty);
      ezInt32 iCount = accessor.GetCount(path);
      ezInt32 iNewIndex = index.ConvertTo<ezInt32>();
      if (iNewIndex >(ezInt32)iCount)
        return false;
    }
    else if (pProp->GetCategory() == ezPropertyCategory::Member)
    {
      ezPropertyPath path(szParentProperty);
      ezVariant value = accessor.GetValue(path);
      if (!value.IsA<ezUuid>())
        return false;

      //  Can't set pointer if it already has a value, need to delete value first.
      if (value.Get<ezUuid>().IsValid())
        return false;
    }
  }

  return InternalCanAdd(pRtti, pParent, szParentProperty, index);
}

bool ezDocumentObjectManager::CanRemove(const ezDocumentObject* pObject) const
{
  const ezDocumentObject* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return false;

  EZ_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  return InternalCanRemove(pObject);
}

bool ezDocumentObjectManager::CanMove(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProperty, const ezVariant& index) const
{
  if (!CanAdd(pObject->GetTypeAccessor().GetType(), pNewParent, szParentProperty, index))
    return false;

  if (!CanRemove(pObject))
    return false;

  if (pNewParent == nullptr)
    pNewParent = GetRootObject();

  if (pObject == pNewParent)
    return false;

  const ezDocumentObject* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return false;

  EZ_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  if (pNewParent != GetRootObject())
  {
    const ezDocumentObject* pNewParentInTree = GetObject(pNewParent->GetGuid());

    if (pNewParentInTree == nullptr)
      return false;

    EZ_ASSERT_DEV(pNewParentInTree == pNewParent, "Tree Corruption!!!");
  }

  const ezDocumentObject* pCurParent = pNewParent->GetParent();

  while (pCurParent)
  {
    if (pCurParent == pObject)
      return false;

    pCurParent = pCurParent->GetParent();
  }

  const ezIReflectedTypeAccessor& accessor = pNewParent->GetTypeAccessor();
  const ezRTTI* pType = accessor.GetType();

  ezPropertyPath path(szParentProperty);
  auto* pProp = ezToolsReflectionUtils::GetPropertyByPath(pType, path);

  if (pProp == nullptr)
    return false;

  if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
  {
    ezInt32 iChildIndex = index.ConvertTo<ezInt32>();
    if (pNewParent == pObject->GetParent())
    {
      // Test whether we are moving before or after ourselves, both of which are not allowed and would not change the tree.
      ezIReflectedTypeAccessor& oldAccessor = pObject->m_pParent->GetTypeAccessor();
      ezInt32 iCurrentIndex = oldAccessor.GetPropertyChildIndex(szParentProperty, pObject->GetGuid()).ConvertTo<ezInt32>();
      if (iChildIndex == iCurrentIndex || iChildIndex == iCurrentIndex + 1)
        return false;
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
