#include <Foundation/PCH.h>
#include <Foundation/Reflection/ReflectionAdapter.h>
#include <Foundation/Reflection/ReflectionUtils.h>

////////////////////////////////////////////////////////////////////////
// ezRttiSerializationContext
////////////////////////////////////////////////////////////////////////

void* ezRttiSerializationContext::CreateObject(const ezUuid& guid, const void* pType)
{
  ezReflectedObjectWrapper* pWrapper = GetObjectByGUID(guid);
  if (pWrapper != nullptr)
  {
    return pWrapper->m_pObject;
  }

  const ezRTTI* pRtti = static_cast<const ezRTTI*>(pType);
  if (!pRtti->GetAllocator()->CanAllocate())
    return nullptr;


  ezReflectedObjectWrapper object;
  object.m_pObject = pRtti->GetAllocator()->Allocate();
  object.m_pType = pType;
  m_Objects.Insert(guid, object);
  m_PtrLookup.Insert(object.m_pObject, guid);

  return object.m_pObject;
}

void ezRttiSerializationContext::DeleteObject(const ezUuid& guid)
{
  ezReflectedObjectWrapper* pObjectWrapper = nullptr;
  if (!m_Objects.TryGetValue(guid, pObjectWrapper))
  {
    EZ_REPORT_FAILURE("Given object guid can't be found and thus can't be deleted!");
    return;
  }

  const ezRTTI* pRtti = static_cast<const ezRTTI*>(pObjectWrapper->m_pType);
  if (!pRtti->GetAllocator()->CanAllocate())
  {
    EZ_REPORT_FAILURE("Given object has no allocator and thus can't be deleted!");
    return;
  }

  void* pObject = pObjectWrapper->m_pObject;
  m_PtrLookup.Remove(pObject);
  m_Objects.Remove(guid);
  m_PendingObjects.Remove(guid);
  pRtti->GetAllocator()->Deallocate(pObject);
}

void ezRttiSerializationContext::RegisterObject(const ezUuid& guid, const void* pType, void* pObject)
{
  ezReflectedObjectWrapper object;
  object.m_pObject = pObject;
  object.m_pType = pType;
  m_Objects.Insert(guid, object);
  m_PtrLookup.Insert(object.m_pObject, guid);
}

ezReflectedObjectWrapper* ezRttiSerializationContext::GetObjectByGUID(const ezUuid& guid) const
{
  ezReflectedObjectWrapper* pObjectWrapper = nullptr;
  if (m_Objects.TryGetValue(guid, pObjectWrapper))
  {
    return pObjectWrapper;
  }
  return nullptr;
}

ezUuid ezRttiSerializationContext::GetObjectGUID(void* pObject) const
{
  ezUuid* pGuid = nullptr;
  if (m_PtrLookup.TryGetValue(pObject, pGuid))
  {
    return *pGuid;
  }
  return ezUuid();
}

ezUuid ezRttiSerializationContext::EnqueObject(void* pObject, const void* pType)
{
  ezUuid guid = GetObjectGUID(pObject);
  if (!guid.IsValid())
  {
    ezReflectedObjectWrapper object;
    object.m_pObject = pObject;
    object.m_pType = pType;
    guid.CreateNewUuid();
    m_Objects.Insert(guid, object);
    m_PtrLookup.Insert(object.m_pObject, guid);
    m_PendingObjects.Insert(guid);
  }
  return guid;
}

ezReflectedObjectWrapper ezRttiSerializationContext::DequeueObject()
{
  ezReflectedObjectWrapper object;
  object.m_pObject = nullptr;
  object.m_pType = nullptr;
  if (m_PendingObjects.IsEmpty())
  {
    return object;
  }
  ezUuid guid = m_PendingObjects.GetIterator().Key();
  m_PendingObjects.Remove(m_PendingObjects.GetIterator());
  if (!m_Objects.TryGetValue(guid, object))
  {
    EZ_REPORT_FAILURE("Failed to retrieve object via guid!");
  }
  return object;
}


////////////////////////////////////////////////////////////////////////
// ezRttiAdapter
////////////////////////////////////////////////////////////////////////

ezRttiAdapter::ezRttiAdapter(ezReflectedSerializationContext* pContext) : ezReflectionAdapter(pContext)
{
}

// Type Info
ezReflectedTypeWrapper ezRttiAdapter::GetTypeInfo(const void* pType) const
{
  ezReflectedTypeWrapper info;

  const ezRTTI* pRtti = static_cast<const ezRTTI*>(pType);
  info.m_Flags = pRtti->GetTypeFlags();
  info.m_pType = pRtti;
  info.m_szName = pRtti->GetTypeName();
  return info;
}

const void* ezRttiAdapter::FindTypeByName(const char* szTypeName) const
{
  return ezRTTI::FindTypeByName(szTypeName);
}

const void* ezRttiAdapter::GetParentType(const void* pType) const
{
  return static_cast<const ezRTTI*>(pType)->GetParentType();
}

bool ezRttiAdapter::IsStandardType(const void* pType) const
{
  const ezRTTI* pRttiType = static_cast<const ezRTTI*>(pType);
  return ezReflectionUtils::IsBasicType(pRttiType) || pRttiType == ezGetStaticRTTI<ezVariant>();
}

// Property Info
ezReflectedPropertyWrapper ezRttiAdapter::GetPropertyInfo(void* pProp) const
{
  ezReflectedPropertyWrapper info;

  ezAbstractProperty* pProp2 = static_cast<ezAbstractProperty*>(pProp);
  info.m_Category = pProp2->GetCategory();
  info.m_pProperty = pProp2;
  info.m_pType = nullptr;
  info.m_szName = pProp2->GetPropertyName();

  switch (info.m_Category)
  {
  case ezPropertyCategory::Constant:
    {
      ezAbstractConstantProperty* pProp3 = static_cast<ezAbstractConstantProperty*>(pProp2);
      info.m_pType = pProp3->GetPropertyType();
      info.m_Flags;
    }
    break;
  case ezPropertyCategory::Member:
    {
      ezAbstractMemberProperty* pProp3 = static_cast<ezAbstractMemberProperty*>(pProp2);
      info.m_pType = pProp3->GetPropertyType();
      info.m_Flags = pProp3->GetFlags();
    }
    break;
  case ezPropertyCategory::Array:
    {
      ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp2);
      info.m_pType = pProp3->GetElementType();
      info.m_Flags = pProp3->GetFlags();
    }
    break;
  case ezPropertyCategory::Set:
    {
      ezAbstractSetProperty* pProp3 = static_cast<ezAbstractSetProperty*>(pProp2);
      info.m_pType = pProp3->GetElementType();
      info.m_Flags = pProp3->GetFlags();
    }
    break;
  default:
    EZ_ASSERT_DEBUG(false, "Not implemented!");
    break;
  }
  return info;
}

ezUInt32 ezRttiAdapter::GetPropertyCount(const void* pType) const
{
  return static_cast<const ezRTTI*>(pType)->GetProperties().GetCount();
}

void* ezRttiAdapter::GetProperty(const void* pType, ezUInt32 uiIndex) const
{
  return static_cast<const ezRTTI*>(pType)->GetProperties()[uiIndex];
}

void* ezRttiAdapter::FindPropertyByName(const void* pType, const char* szPropName) const
{
  return static_cast<const ezRTTI*>(pType)->FindPropertyByName(szPropName, true);
}

// Property Access
ezVariant ezRttiAdapter::GetPropertyValue(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) const
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Constant:
    {
      ezAbstractConstantProperty* pProp3 = static_cast<ezAbstractConstantProperty*>(pProp);
      return pProp3->GetConstant();
    }
    break;
  case ezPropertyCategory::Member:
    {
      ezAbstractMemberProperty* pProp3 = static_cast<ezAbstractMemberProperty*>(pProp);
      return ezReflectionUtils::GetMemberPropertyValue(pProp3, object.m_pObject);
    }
    break;
  case ezPropertyCategory::Array:
    {
      ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
      return ezReflectionUtils::GetArrayPropertyValue(pProp3, object.m_pObject, index.ConvertTo<ezUInt32>());
    }
    break;
  default:
    EZ_ASSERT_DEBUG(false, "Not implemented!");
    break;
  }
  return ezVariant();
}

void ezRttiAdapter::SetPropertyValue(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, const ezVariant& value)
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      ezAbstractMemberProperty* pProp3 = static_cast<ezAbstractMemberProperty*>(pProp);
      ezReflectionUtils::SetMemberPropertyValue(pProp3, object.m_pObject, value);
    }
    break;
  case ezPropertyCategory::Array:
    {
      ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
      ezReflectionUtils::SetArrayPropertyValue(pProp3, object.m_pObject, index.ConvertTo<ezUInt32>(), value);
    }
    break;
  default:
    EZ_ASSERT_DEBUG(false, "Not implemented!");
    break;
  }
}

void ezRttiAdapter::GetPropertyObject(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, ezReflectedObjectWrapper& value) const
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      ezAbstractMemberProperty* pProp3 = static_cast<ezAbstractMemberProperty*>(pProp);
      pProp3->GetValuePtr(object.m_pObject, value.m_pObject);
    }
    break;
  case ezPropertyCategory::Array:
    {
      ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
      pProp3->GetValue(object.m_pObject, index.ConvertTo<ezUInt32>(), value.m_pObject);
    }
    break;
  default:
    EZ_ASSERT_DEBUG(false, "Not implemented!");
    break;
  }
}

void ezRttiAdapter::SetPropertyObject(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, const ezReflectedObjectWrapper& value)
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      ezAbstractMemberProperty* pProp3 = static_cast<ezAbstractMemberProperty*>(pProp);
      pProp3->SetValuePtr(object.m_pObject, value.m_pObject);
    }
    break;
  case ezPropertyCategory::Array:
    {
      ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
      pProp3->SetValue(object.m_pObject, index.ConvertTo<ezUInt32>(), value.m_pObject);
    }
    break;
  default:
    EZ_ASSERT_DEBUG(false, "Not implemented!");
    break;
  }
}

bool ezRttiAdapter::CanGetDirectPropertyPointer(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) const
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  if (pProp->GetCategory() != ezPropertyCategory::Member)
    return false;

  ezAbstractMemberProperty* pProp2 = static_cast<ezAbstractMemberProperty*>(pProp);
  return pProp2->GetPropertyPointer(object.m_pObject) != nullptr;
}

ezReflectedObjectWrapper ezRttiAdapter::GetDirectPropertyPointer(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index)
{
  ezReflectedObjectWrapper subObject;
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Member, "");

  ezAbstractMemberProperty* pProp2 = static_cast<ezAbstractMemberProperty*>(pProp);

  subObject.m_pObject = pProp2->GetPropertyPointer(object.m_pObject);
  subObject.m_pType = prop.m_pType;
  return subObject;
}

// Array
ezUInt32 ezRttiAdapter::GetArrayElementCount(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop) const
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Array, "GetArrayElementCount can only be called for array properties!");

  ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
  return pProp3->GetCount(object.m_pObject);
}

void ezRttiAdapter::SetArrayElementCount(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, ezUInt32 uiCount)
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Array, "SetArrayElementCount can only be called for array properties!");

  ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
  pProp3->SetCount(object.m_pObject, uiCount);
}

// Set
void ezRttiAdapter::GetSetContent(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, ezHybridArray<ezVariant, 16>& out_Keys) const
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Set, "GetSetContent can only be called for set properties!");

  ezAbstractSetProperty* pProp3 = static_cast<ezAbstractSetProperty*>(pProp);
  pProp3->GetValues(object.m_pObject, out_Keys);
}

void ezRttiAdapter::SetSetContent(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezHybridArray<ezVariant, 16>& keys)
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Set, "GetSetContent can only be called for set properties!");

  ezAbstractSetProperty* pProp3 = static_cast<ezAbstractSetProperty*>(pProp);
  pProp3->Clear(object.m_pObject);

  for (const ezVariant& var : keys)
  {
    ezReflectionUtils::InsertSetPropertyValue(pProp3, object.m_pObject, var);
  }
}

// Allocate
bool ezRttiAdapter::CanCreateObject(const void* pType)
{
  const ezRTTI* pRtti = static_cast<const ezRTTI*>(pType);
  return pRtti->GetAllocator()->CanAllocate();
}
ezReflectedObjectWrapper ezRttiAdapter::CreateObject(const void* pType)
{
  ezReflectedObjectWrapper object;

  const ezRTTI* pRtti = static_cast<const ezRTTI*>(pType);
  object.m_pObject = pRtti->GetAllocator()->Allocate();
  object.m_pType = pRtti;

  return object;
}
void ezRttiAdapter::DeleteObject(ezReflectedObjectWrapper& object)
{
  const ezRTTI* pRtti = static_cast<const ezRTTI*>(object.m_pType);
  pRtti->GetAllocator()->Deallocate(object.m_pObject);
  object.m_pObject = nullptr;
}

