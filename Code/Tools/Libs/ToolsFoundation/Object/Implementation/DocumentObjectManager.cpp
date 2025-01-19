#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager
////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentRoot, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Children", m_RootObjects)->AddFlags(ezPropertyFlags::PointerOwner),
    EZ_ARRAY_MEMBER_PROPERTY("TempObjects", m_TempObjects)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezTemporaryAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezDocumentRootObject::InsertSubObject(ezDocumentObject* pObject, ezStringView sProperty, const ezVariant& index)
{
  if (sProperty.IsEmpty())
    sProperty = "Children";
  return ezDocumentObject::InsertSubObject(pObject, sProperty, index);
}

void ezDocumentRootObject::RemoveSubObject(ezDocumentObject* pObject)
{
  return ezDocumentObject::RemoveSubObject(pObject);
}

ezVariant ezDocumentObjectPropertyEvent::getInsertIndex() const
{
  if (m_EventType == Type::PropertyMoved)
  {
    const ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    const ezRTTI* pType = accessor.GetType();
    auto* pProp = pType->FindPropertyByName(m_sProperty);
    if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
    {
      ezInt32 iCurrentIndex = m_OldIndex.ConvertTo<ezInt32>();
      ezInt32 iNewIndex = m_NewIndex.ConvertTo<ezInt32>();
      // Move after oneself?
      if (iNewIndex > iCurrentIndex)
      {
        iNewIndex -= 1;
        return ezVariant(iNewIndex);
      }
    }
  }
  return m_NewIndex;
}

ezDocumentObjectManager::Storage::Storage(const ezRTTI* pRootType)
  : m_RootObject(pRootType)
{
}

ezDocumentObjectManager::ezDocumentObjectManager(const ezRTTI* pRootType)
{
  auto pStorage = EZ_DEFAULT_NEW(Storage, pRootType);
  pStorage->m_RootObject.m_pDocumentObjectManager = this;
  SwapStorage(pStorage);
}

ezDocumentObjectManager::~ezDocumentObjectManager()
{
  if (m_pObjectStorage->GetRefCount() == 1)
  {
    EZ_ASSERT_DEV(m_pObjectStorage->m_GuidToObject.IsEmpty(), "Not all objects have been destroyed!");
  }
}

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Object Construction / Destruction
////////////////////////////////////////////////////////////////////////

ezDocumentObject* ezDocumentObjectManager::CreateObject(const ezRTTI* pRtti, ezUuid guid)
{
  EZ_ASSERT_DEV(pRtti != nullptr, "Unknown RTTI type");

  ezDocumentObject* pObject = InternalCreateObject(pRtti);
  // In case the storage is swapped, objects should still be created in their original document manager.
  pObject->m_pDocumentObjectManager = m_pObjectStorage->m_RootObject.GetDocumentObjectManager();

  if (guid.IsValid())
    pObject->m_Guid = guid;
  else
    pObject->m_Guid = ezUuid::MakeUuid();

  PatchEmbeddedClassObjectsInternal(pObject, pRtti, false);

  ezDocumentObjectEvent e;
  e.m_pObject = pObject;
  e.m_EventType = ezDocumentObjectEvent::Type::AfterObjectCreated;
  m_pObjectStorage->m_ObjectEvents.Broadcast(e);

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
  m_pObjectStorage->m_ObjectEvents.Broadcast(e);

  InternalDestroyObject(pObject);
}

void ezDocumentObjectManager::DestroyAllObjects()
{
  for (auto child : m_pObjectStorage->m_RootObject.m_Children)
  {
    DestroyObject(child);
  }

  m_pObjectStorage->m_RootObject.m_Children.Clear();
  m_pObjectStorage->m_GuidToObject.Clear();
}

void ezDocumentObjectManager::PatchEmbeddedClassObjects(const ezDocumentObject* pObject) const
{
  // Functional should be callable from anywhere but will of course have side effects.
  const_cast<ezDocumentObjectManager*>(this)->PatchEmbeddedClassObjectsInternal(
    const_cast<ezDocumentObject*>(pObject), pObject->GetTypeAccessor().GetType(), true);
}

const ezDocumentObject* ezDocumentObjectManager::GetObject(const ezUuid& guid) const
{
  const ezDocumentObject* pObject = nullptr;
  if (m_pObjectStorage->m_GuidToObject.TryGetValue(guid, pObject))
  {
    return pObject;
  }
  else if (guid == m_pObjectStorage->m_RootObject.GetGuid())
    return &m_pObjectStorage->m_RootObject;
  return nullptr;
}

ezDocumentObject* ezDocumentObjectManager::GetObject(const ezUuid& guid)
{
  return const_cast<ezDocumentObject*>(((const ezDocumentObjectManager*)this)->GetObject(guid));
}

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Property Change
////////////////////////////////////////////////////////////////////////

ezStatus ezDocumentObjectManager::SetValue(ezDocumentObject* pObject, ezStringView sProperty, const ezVariant& newValue, ezVariant index)
{
  EZ_ASSERT_DEBUG(pObject, "Object must not be null.");
  ezIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  ezVariant oldValue = accessor.GetValue(sProperty, index);

  if (!accessor.SetValue(sProperty, newValue, index))
  {
    return ezStatus(ezFmt("Set Property: The property '{0}' does not exist or value type does not match", sProperty));
  }

  ezDocumentObjectPropertyEvent e;
  e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertySet;
  e.m_pObject = pObject;
  e.m_OldValue = oldValue;
  e.m_NewValue = newValue;
  e.m_sProperty = sProperty;
  e.m_NewIndex = index;

  // Allow a recursion depth of 2 for property setters. This allowed for two levels of side-effects on property setters.
  m_pObjectStorage->m_PropertyEvents.Broadcast(e, 2);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDocumentObjectManager::InsertValue(ezDocumentObject* pObject, ezStringView sProperty, const ezVariant& newValue, ezVariant index)
{
  ezIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  if (!accessor.InsertValue(sProperty, index, newValue))
  {
    if (!accessor.GetType()->FindPropertyByName(sProperty))
    {
      return ezStatus(ezFmt("Insert Property: The property '{0}' does not exist", sProperty));
    }
    return ezStatus(ezFmt("Insert Property: The property '{0}' already has the key '{1}'", sProperty, index));
  }

  ezDocumentObjectPropertyEvent e;
  e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyInserted;
  e.m_pObject = pObject;
  e.m_NewValue = newValue;
  e.m_NewIndex = index;
  e.m_sProperty = sProperty;

  m_pObjectStorage->m_PropertyEvents.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDocumentObjectManager::RemoveValue(ezDocumentObject* pObject, ezStringView sProperty, ezVariant index)
{
  ezIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  ezVariant oldValue = accessor.GetValue(sProperty, index);

  if (!accessor.RemoveValue(sProperty, index))
  {
    return ezStatus(ezFmt("Remove Property: The index '{0}' in property '{1}' does not exist!", index.ConvertTo<ezString>(), sProperty));
  }

  ezDocumentObjectPropertyEvent e;
  e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyRemoved;
  e.m_pObject = pObject;
  e.m_OldValue = oldValue;
  e.m_OldIndex = index;
  e.m_sProperty = sProperty;

  m_pObjectStorage->m_PropertyEvents.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDocumentObjectManager::MoveValue(ezDocumentObject* pObject, ezStringView sProperty, const ezVariant& oldIndex, const ezVariant& newIndex)
{
  if (!oldIndex.CanConvertTo<ezInt32>() || !newIndex.CanConvertTo<ezInt32>())
    return ezStatus("Move Property: Invalid indices provided.");

  ezIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  ezInt32 iCount = accessor.GetCount(sProperty);
  if (iCount < 0)
    return ezStatus("Move Property: Invalid property.");
  if (oldIndex.ConvertTo<ezInt32>() < 0 || oldIndex.ConvertTo<ezInt32>() >= iCount)
    return ezStatus(ezFmt("Move Property: Invalid old index '{0}'.", oldIndex.ConvertTo<ezInt32>()));
  if (newIndex.ConvertTo<ezInt32>() < 0 || newIndex.ConvertTo<ezInt32>() > iCount)
    return ezStatus(ezFmt("Move Property: Invalid new index '{0}'.", newIndex.ConvertTo<ezInt32>()));

  if (!accessor.MoveValue(sProperty, oldIndex, newIndex))
    return ezStatus("Move Property: Move value failed.");

  {
    ezDocumentObjectPropertyEvent e;
    e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertyMoved;
    e.m_pObject = pObject;
    e.m_OldIndex = oldIndex;
    e.m_NewIndex = newIndex;
    e.m_sProperty = sProperty;
    e.m_NewValue = accessor.GetValue(sProperty, e.getInsertIndex());
    // NewValue can be invalid if an invalid variant in a variant array is moved
    // EZ_ASSERT_DEV(e.m_NewValue.IsValid(), "Value at new pos should be valid now, index missmatch?");
    m_pObjectStorage->m_PropertyEvents.Broadcast(e);
  }

  return ezStatus(EZ_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Structure Change
////////////////////////////////////////////////////////////////////////

void ezDocumentObjectManager::AddObject(ezDocumentObject* pObject, ezDocumentObject* pParent, ezStringView sParentProperty, ezVariant index)
{
  if (pParent == nullptr)
    pParent = &m_pObjectStorage->m_RootObject;
  if (pParent == &m_pObjectStorage->m_RootObject && sParentProperty.IsEmpty())
    sParentProperty = "Children";

  EZ_ASSERT_DEV(pObject->GetGuid().IsValid(), "Object Guid invalid! Object was not created via an ezObjectManagerBase!");
  EZ_ASSERT_DEV(CanAdd(pObject->GetTypeAccessor().GetType(), pParent, sParentProperty, index).Succeeded(), "Trying to execute invalid add!");

  InternalAddObject(pObject, pParent, sParentProperty, index);
}

void ezDocumentObjectManager::RemoveObject(ezDocumentObject* pObject)
{
  EZ_ASSERT_DEV(CanRemove(pObject).Succeeded(), "Trying to execute invalid remove!");
  InternalRemoveObject(pObject);
}

void ezDocumentObjectManager::MoveObject(ezDocumentObject* pObject, ezDocumentObject* pNewParent, ezStringView sParentProperty, ezVariant index)
{
  EZ_ASSERT_DEV(CanMove(pObject, pNewParent, sParentProperty, index).Succeeded(), "Trying to execute invalid move!");

  InternalMoveObject(pNewParent, pObject, sParentProperty, index);
}


////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Structure Change Test
////////////////////////////////////////////////////////////////////////

ezStatus ezDocumentObjectManager::CanAdd(
  const ezRTTI* pRtti, const ezDocumentObject* pParent, ezStringView sParentProperty, const ezVariant& index) const
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
    auto* pProp = pType->FindPropertyByName(sParentProperty);
    if (pProp == nullptr)
      return ezStatus(ezFmt("Property '{0}' could not be found in type '{1}'", sParentProperty, pType->GetTypeName()));

    const bool bIsValueType = ezReflectionUtils::IsValueType(pProp);

    if (bIsValueType || pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
    {
      return ezStatus("Need to use 'InsertValue' action instead.");
    }
    else if (pProp->GetFlags().IsSet(ezPropertyFlags::Class))
    {
      if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        if (!pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          return ezStatus(ezFmt("Cannot add object to the pointer property '{0}' as it does not hold ownership.", sParentProperty));

        if (!pRtti->IsDerivedFrom(pProp->GetSpecificType()))
          return ezStatus(ezFmt("Cannot add object to the pointer property '{0}' as its type '{1}' is not derived from the property type '{2}'!",
            sParentProperty, pRtti->GetTypeName(), pProp->GetSpecificType()->GetTypeName()));
      }
      else
      {
        if (pRtti != pProp->GetSpecificType())
          return ezStatus(ezFmt("Cannot add object to the property '{0}' as its type '{1}' does not match the property type '{2}'!", sParentProperty,
            pRtti->GetTypeName(), pProp->GetSpecificType()->GetTypeName()));
      }
    }

    if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
    {
      ezInt32 iCount = accessor.GetCount(sParentProperty);
      if (!index.CanConvertTo<ezInt32>())
      {
        return ezStatus(ezFmt("Cannot add object to the property '{0}', the given index is an invalid ezVariant (Either use '-1' to append "
                              "or a valid index).",
          sParentProperty));
      }
      ezInt32 iNewIndex = index.ConvertTo<ezInt32>();
      if (iNewIndex > (ezInt32)iCount)
        return ezStatus(ezFmt(
          "Cannot add object to its new location '{0}' is out of the bounds of the parent's property range '{1}'!", iNewIndex, (ezInt32)iCount));
      if (iNewIndex < 0 && iNewIndex != -1)
        return ezStatus(ezFmt("Cannot add object to the property '{0}', the index '{1}' is not valid (Either use '-1' to append or a valid index).",
          sParentProperty, iNewIndex));
    }
    if (pProp->GetCategory() == ezPropertyCategory::Map)
    {
      if (!index.IsA<ezString>())
        return ezStatus(ezFmt("Cannot add object to the map property '{0}' as its index type is not a string.", sParentProperty));
      ezVariant value = accessor.GetValue(sParentProperty, index);
      if (value.IsValid() && value.IsA<ezUuid>())
      {
        ezUuid guid = value.Get<ezUuid>();
        if (guid.IsValid())
          return ezStatus(
            ezFmt("Cannot add object to the map property '{0}' at key '{1}'. Delete old value first.", sParentProperty, index.Get<ezString>()));
      }
    }
    else if (pProp->GetCategory() == ezPropertyCategory::Member)
    {
      if (!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
        return ezStatus("Embedded classes cannot be changed manually.");

      ezVariant value = accessor.GetValue(sParentProperty);
      if (!value.IsA<ezUuid>())
        return ezStatus("Property is not a pointer and thus can't be added to.");

      if (value.Get<ezUuid>().IsValid())
        return ezStatus("Can't set pointer if it already has a value, need to delete value first.");
    }
  }

  return InternalCanAdd(pRtti, pParent, sParentProperty, index);
}

ezStatus ezDocumentObjectManager::CanRemove(const ezDocumentObject* pObject) const
{
  const ezDocumentObject* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return ezStatus("Object is not part of the object manager!");

  if (pObject->GetParent())
  {
    const ezAbstractProperty* pProp = pObject->GetParentPropertyType();
    EZ_ASSERT_DEV(pProp != nullptr, "Parent property should always be valid!");
    if (pProp->GetCategory() == ezPropertyCategory::Member && !pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      return ezStatus("Non pointer members can't be deleted!");
  }
  EZ_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  return InternalCanRemove(pObject);
}

ezStatus ezDocumentObjectManager::CanMove(
  const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, ezStringView sParentProperty, const ezVariant& index) const
{
  EZ_SUCCEED_OR_RETURN(CanAdd(pObject->GetTypeAccessor().GetType(), pNewParent, sParentProperty, index));

  EZ_SUCCEED_OR_RETURN(CanRemove(pObject));

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

  auto* pProp = pType->FindPropertyByName(sParentProperty);

  if (pProp == nullptr)
    return ezStatus(ezFmt("Property '{0}' could not be found in type '{1}'", sParentProperty, pType->GetTypeName()));

  if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
  {
    ezInt32 iChildIndex = index.ConvertTo<ezInt32>();
    if (iChildIndex == -1)
    {
      iChildIndex = pNewParent->GetTypeAccessor().GetCount(sParentProperty);
    }

    if (pNewParent == pObject->GetParent())
    {
      // Test whether we are moving before or after ourselves, both of which are not allowed and would not change the tree.
      ezIReflectedTypeAccessor& oldAccessor = pObject->m_pParent->GetTypeAccessor();
      ezInt32 iCurrentIndex = oldAccessor.GetPropertyChildIndex(sParentProperty, pObject->GetGuid()).ConvertTo<ezInt32>();
      if (iChildIndex == iCurrentIndex || iChildIndex == iCurrentIndex + 1)
        return ezStatus("Can't move object onto itself!");
    }
  }
  if (pProp->GetCategory() == ezPropertyCategory::Map)
  {
    if (!index.IsA<ezString>())
      return ezStatus(ezFmt("Cannot add object to the map property '{0}' as its index type is not a string.", sParentProperty));
    ezVariant value = accessor.GetValue(sParentProperty, index);
    if (value.IsValid() && value.IsA<ezUuid>())
    {
      ezUuid guid = value.Get<ezUuid>();
      if (guid.IsValid())
        return ezStatus(
          ezFmt("Cannot add object to the map property '{0}' at key '{1}'. Delete old value first.", sParentProperty, index.Get<ezString>()));
    }
  }

  if (pNewParent == GetRootObject())
    pNewParent = nullptr;

  return InternalCanMove(pObject, pNewParent, sParentProperty, index);
}

ezStatus ezDocumentObjectManager::CanSelect(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "pObject must be valid");

  const ezDocumentObject* pOwnObject = GetObject(pObject->GetGuid());
  if (pOwnObject == nullptr)
    return ezStatus(
      ezFmt("Object of type '{0}' is not part of the document and can't be selected", pObject->GetTypeAccessor().GetType()->GetTypeName()));

  return InternalCanSelect(pObject);
}


bool ezDocumentObjectManager::IsUnderRootProperty(ezStringView sRootProperty, const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEBUG(m_pObjectStorage->m_RootObject.GetDocumentObjectManager() == pObject->GetDocumentObjectManager(), "Passed in object does not belong to this object manager.");
  while (pObject->GetParent() != GetRootObject())
  {
    pObject = pObject->GetParent();
  }
  return sRootProperty == pObject->GetParentProperty();
}


bool ezDocumentObjectManager::IsUnderRootProperty(ezStringView sRootProperty, const ezDocumentObject* pParent, ezStringView sParentProperty) const
{
  EZ_ASSERT_DEBUG(pParent == nullptr || m_pObjectStorage->m_RootObject.GetDocumentObjectManager() == pParent->GetDocumentObjectManager(), "Passed in object does not belong to this object manager.");
  if (pParent == nullptr || pParent == GetRootObject())
  {
    return sParentProperty == sRootProperty;
  }
  return IsUnderRootProperty(sRootProperty, pParent);
}

bool ezDocumentObjectManager::IsTemporary(const ezDocumentObject* pObject) const
{
  return IsUnderRootProperty("TempObjects", pObject);
}

bool ezDocumentObjectManager::IsTemporary(const ezDocumentObject* pParent, ezStringView sParentProperty) const
{
  return IsUnderRootProperty("TempObjects", pParent, sParentProperty);
}

ezSharedPtr<ezDocumentObjectManager::Storage> ezDocumentObjectManager::SwapStorage(ezSharedPtr<ezDocumentObjectManager::Storage> pNewStorage)
{
  EZ_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  auto retVal = m_pObjectStorage;

  m_StructureEventsUnsubscriber.Unsubscribe();
  m_PropertyEventsUnsubscriber.Unsubscribe();
  m_ObjectEventsUnsubscriber.Unsubscribe();

  m_pObjectStorage = pNewStorage;

  m_pObjectStorage->m_StructureEvents.AddEventHandler([this](const ezDocumentObjectStructureEvent& e)
    { m_StructureEvents.Broadcast(e); }, m_StructureEventsUnsubscriber);
  m_pObjectStorage->m_PropertyEvents.AddEventHandler([this](const ezDocumentObjectPropertyEvent& e)
    { m_PropertyEvents.Broadcast(e, 2); }, m_PropertyEventsUnsubscriber);
  m_pObjectStorage->m_ObjectEvents.AddEventHandler([this](const ezDocumentObjectEvent& e)
    { m_ObjectEvents.Broadcast(e); }, m_ObjectEventsUnsubscriber);

  return retVal;
}

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Private Functions
////////////////////////////////////////////////////////////////////////

void ezDocumentObjectManager::InternalAddObject(ezDocumentObject* pObject, ezDocumentObject* pParent, ezStringView sParentProperty, ezVariant index)
{
  ezDocumentObjectStructureEvent e;
  e.m_pDocument = m_pObjectStorage->m_pDocument;
  e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeObjectAdded;
  e.m_pObject = pObject;
  e.m_pPreviousParent = nullptr;
  e.m_pNewParent = pParent;
  e.m_sParentProperty = sParentProperty;
  e.m_NewPropertyIndex = index;

  if (e.m_NewPropertyIndex.CanConvertTo<ezInt32>() && e.m_NewPropertyIndex.ConvertTo<ezInt32>() == -1)
  {
    ezIReflectedTypeAccessor& accessor = pParent->GetTypeAccessor();
    e.m_NewPropertyIndex = accessor.GetCount(sParentProperty);
  }
  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  pParent->InsertSubObject(pObject, sParentProperty, e.m_NewPropertyIndex);
  RecursiveAddGuids(pObject);

  e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectAdded;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);
}

void ezDocumentObjectManager::InternalRemoveObject(ezDocumentObject* pObject)
{
  ezDocumentObjectStructureEvent e;
  e.m_pDocument = m_pObjectStorage->m_pDocument;
  e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = nullptr;
  e.m_sParentProperty = pObject->m_sParentProperty;
  e.m_OldPropertyIndex = pObject->GetPropertyIndex();
  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  pObject->m_pParent->RemoveSubObject(pObject);
  RecursiveRemoveGuids(pObject);

  e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectRemoved;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);
}

void ezDocumentObjectManager::InternalMoveObject(
  ezDocumentObject* pNewParent, ezDocumentObject* pObject, ezStringView sParentProperty, ezVariant index)
{
  if (pNewParent == nullptr)
    pNewParent = &m_pObjectStorage->m_RootObject;

  ezDocumentObjectStructureEvent e;
  e.m_pDocument = m_pObjectStorage->m_pDocument;
  e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeObjectMoved;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = pNewParent;
  e.m_sParentProperty = sParentProperty;
  e.m_OldPropertyIndex = pObject->GetPropertyIndex();
  e.m_NewPropertyIndex = index;
  if (e.m_NewPropertyIndex.CanConvertTo<ezInt32>() && e.m_NewPropertyIndex.ConvertTo<ezInt32>() == -1)
  {
    ezIReflectedTypeAccessor& accessor = pNewParent->GetTypeAccessor();
    e.m_NewPropertyIndex = accessor.GetCount(sParentProperty);
  }

  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  ezVariant newIndex = e.getInsertIndex();

  pObject->m_pParent->RemoveSubObject(pObject);
  pNewParent->InsertSubObject(pObject, sParentProperty, newIndex);

  e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectMoved;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectMoved2;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);
}

void ezDocumentObjectManager::RecursiveAddGuids(ezDocumentObject* pObject)
{
  m_pObjectStorage->m_GuidToObject[pObject->m_Guid] = pObject;

  for (ezUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveAddGuids(pObject->GetChildren()[c]);
}

void ezDocumentObjectManager::RecursiveRemoveGuids(ezDocumentObject* pObject)
{
  m_pObjectStorage->m_GuidToObject.Remove(pObject->m_Guid);

  for (ezUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveRemoveGuids(pObject->GetChildren()[c]);
}

void ezDocumentObjectManager::PatchEmbeddedClassObjectsInternal(ezDocumentObject* pObject, const ezRTTI* pType, bool addToDoc)
{
  const ezRTTI* pParent = pType->GetParentType();
  if (pParent != nullptr)
    PatchEmbeddedClassObjectsInternal(pObject, pParent, addToDoc);

  ezIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  const ezUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (ezUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const ezAbstractProperty* pProperty = pType->GetProperties()[i];
    const ezVariantTypeInfo* pInfo = ezVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pProperty->GetSpecificType());

    if (pProperty->GetCategory() == ezPropertyCategory::Member && pProperty->GetFlags().IsSet(ezPropertyFlags::Class) && !pInfo &&
        !pProperty->GetFlags().IsSet(ezPropertyFlags::Pointer))
    {
      ezUuid value = accessor.GetValue(pProperty->GetPropertyName()).Get<ezUuid>();
      EZ_ASSERT_DEV(addToDoc || !value.IsValid(), "If addToDoc is false, the current value must be invalid!");
      if (value.IsValid())
      {
        ezDocumentObject* pEmbeddedObject = GetObject(value);
        if (pEmbeddedObject)
        {
          if (pEmbeddedObject->GetTypeAccessor().GetType() == pProperty->GetSpecificType())
            continue;
          else
          {
            // Type mismatch, delete old.
            InternalRemoveObject(pEmbeddedObject);
          }
        }
      }

      // Create new
      ezStringBuilder sTemp;
      ezConversionUtils::ToString(pObject->GetGuid(), sTemp);
      sTemp.Append("/", pProperty->GetPropertyName());
      const ezUuid subObjectGuid = ezUuid::MakeStableUuidFromString(sTemp);
      ezDocumentObject* pEmbeddedObject = CreateObject(pProperty->GetSpecificType(), subObjectGuid);
      if (addToDoc)
      {
        InternalAddObject(pEmbeddedObject, pObject, pProperty->GetPropertyName(), ezVariant());
      }
      else
      {
        pObject->InsertSubObject(pEmbeddedObject, pProperty->GetPropertyName(), ezVariant());
      }
    }
  }
}


const ezAbstractProperty* ezDocumentObjectStructureEvent::GetProperty() const
{
  return m_pObject->GetParentPropertyType();
}

ezVariant ezDocumentObjectStructureEvent::getInsertIndex() const
{
  if ((m_EventType == Type::BeforeObjectMoved || m_EventType == Type::AfterObjectMoved || m_EventType == Type::AfterObjectMoved2) &&
      m_pNewParent == m_pPreviousParent)
  {
    const ezIReflectedTypeAccessor& accessor = m_pPreviousParent->GetTypeAccessor();
    const ezRTTI* pType = accessor.GetType();
    auto* pProp = pType->FindPropertyByName(m_sParentProperty);
    if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
    {
      ezInt32 iCurrentIndex = m_OldPropertyIndex.ConvertTo<ezInt32>();
      ezInt32 iNewIndex = m_NewPropertyIndex.ConvertTo<ezInt32>();
      // Move after oneself?
      if (iNewIndex > iCurrentIndex)
      {
        iNewIndex -= 1;
        return ezVariant(iNewIndex);
      }
    }
  }
  return m_NewPropertyIndex;
}
